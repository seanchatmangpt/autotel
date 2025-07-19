#include "../c_src/pm7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Healthcare process mining demo
int main()
{
  printf("=== 7T Healthcare Process Mining Demo ===\n");
  printf("Analyzing patient journeys through hospital system\n\n");

  // Set memory limit (2GB for healthcare data)
  pm7t_set_memory_limit(2ULL * 1024 * 1024 * 1024);

  // Create event log for healthcare processes
  EventLog *event_log = pm7t_create_event_log(50000);
  if (!event_log)
  {
    fprintf(stderr, "Failed to create healthcare event log\n");
    return 1;
  }

  // Healthcare activity definitions
  const char *activities[] = {
      "Patient Registration",
      "Triage Assessment",
      "Emergency Care",
      "Lab Tests",
      "Imaging Scan",
      "Doctor Consultation",
      "Specialist Referral",
      "Surgery Preparation",
      "Surgery",
      "Post-Op Care",
      "Medication Prescription",
      "Pharmacy Dispense",
      "Physical Therapy",
      "Discharge Planning",
      "Patient Discharge",
      "Follow-up Appointment",
      "Insurance Processing",
      "Billing",
      "Payment Collection"};

  printf("Generating synthetic healthcare process data...\n");

  // Generate synthetic healthcare process data
  uint64_t base_time = time(NULL) * 1000000000ULL;
  uint32_t patient_id = 1;
  srand(42); // For reproducible results

  // Generate different patient journey types
  for (int journey_type = 0; journey_type < 8; journey_type++)
  {
    int instances = (journey_type == 0) ? 100 : 50; // More instances for routine care

    for (int instance = 0; instance < instances; instance++)
    {
      uint64_t journey_start = base_time + (patient_id * 3600000000000ULL); // 1 hour between patients
      uint64_t current_time = journey_start;

      switch (journey_type)
      {
      case 0:                                                                            // Routine care (100 instances)
        pm7t_add_event(event_log, patient_id, 0, current_time += 30000000000, 101, 30);  // Registration (30 min)
        pm7t_add_event(event_log, patient_id, 1, current_time += 45000000000, 102, 45);  // Triage (45 min)
        pm7t_add_event(event_log, patient_id, 5, current_time += 72000000000, 103, 120); // Doctor consult (2 hours)
        pm7t_add_event(event_log, patient_id, 3, current_time += 54000000000, 104, 90);  // Lab tests (90 min)
        pm7t_add_event(event_log, patient_id, 10, current_time += 36000000000, 105, 60); // Prescription (60 min)
        pm7t_add_event(event_log, patient_id, 11, current_time += 27000000000, 106, 45); // Pharmacy (45 min)
        pm7t_add_event(event_log, patient_id, 14, current_time += 18000000000, 107, 30); // Discharge (30 min)
        break;

      case 1:                                                                             // Emergency care (50 instances)
        pm7t_add_event(event_log, patient_id, 0, current_time += 15000000000, 101, 20);   // Registration (15 min)
        pm7t_add_event(event_log, patient_id, 1, current_time += 18000000000, 102, 30);   // Triage (18 min)
        pm7t_add_event(event_log, patient_id, 2, current_time += 108000000000, 103, 180); // Emergency care (3 hours)
        pm7t_add_event(event_log, patient_id, 3, current_time += 36000000000, 104, 60);   // Lab tests (60 min)
        pm7t_add_event(event_log, patient_id, 4, current_time += 72000000000, 105, 120);  // Imaging (2 hours)
        pm7t_add_event(event_log, patient_id, 5, current_time += 54000000000, 106, 90);   // Doctor consult (90 min)
        pm7t_add_event(event_log, patient_id, 10, current_time += 27000000000, 107, 45);  // Prescription (45 min)
        pm7t_add_event(event_log, patient_id, 11, current_time += 18000000000, 108, 30);  // Pharmacy (30 min)
        pm7t_add_event(event_log, patient_id, 14, current_time += 15000000000, 109, 25);  // Discharge (15 min)
        break;

      case 2:                                                                             // Surgery pathway (50 instances)
        pm7t_add_event(event_log, patient_id, 0, current_time += 18000000000, 101, 30);   // Registration (30 min)
        pm7t_add_event(event_log, patient_id, 1, current_time += 27000000000, 102, 45);   // Triage (45 min)
        pm7t_add_event(event_log, patient_id, 5, current_time += 72000000000, 103, 120);  // Doctor consult (2 hours)
        pm7t_add_event(event_log, patient_id, 6, current_time += 54000000000, 104, 90);   // Specialist referral (90 min)
        pm7t_add_event(event_log, patient_id, 3, current_time += 72000000000, 105, 120);  // Lab tests (2 hours)
        pm7t_add_event(event_log, patient_id, 4, current_time += 108000000000, 106, 180); // Imaging (3 hours)
        pm7t_add_event(event_log, patient_id, 7, current_time += 36000000000, 107, 60);   // Surgery prep (60 min)
        pm7t_add_event(event_log, patient_id, 8, current_time += 144000000000, 108, 240); // Surgery (4 hours)
        pm7t_add_event(event_log, patient_id, 9, current_time += 216000000000, 109, 360); // Post-op care (6 hours)
        pm7t_add_event(event_log, patient_id, 12, current_time += 72000000000, 110, 120); // Physical therapy (2 hours)
        pm7t_add_event(event_log, patient_id, 13, current_time += 36000000000, 111, 60);  // Discharge planning (60 min)
        pm7t_add_event(event_log, patient_id, 14, current_time += 18000000000, 112, 30);  // Discharge (30 min)
        pm7t_add_event(event_log, patient_id, 15, current_time += 27000000000, 113, 45);  // Follow-up (45 min)
        break;

      case 3:                                                                             // Complex case with multiple specialists (50 instances)
        pm7t_add_event(event_log, patient_id, 0, current_time += 18000000000, 101, 30);   // Registration (30 min)
        pm7t_add_event(event_log, patient_id, 1, current_time += 27000000000, 102, 45);   // Triage (45 min)
        pm7t_add_event(event_log, patient_id, 5, current_time += 72000000000, 103, 120);  // Doctor consult (2 hours)
        pm7t_add_event(event_log, patient_id, 6, current_time += 54000000000, 104, 90);   // Specialist 1 (90 min)
        pm7t_add_event(event_log, patient_id, 3, current_time += 72000000000, 105, 120);  // Lab tests (2 hours)
        pm7t_add_event(event_log, patient_id, 4, current_time += 108000000000, 106, 180); // Imaging (3 hours)
        pm7t_add_event(event_log, patient_id, 6, current_time += 54000000000, 107, 90);   // Specialist 2 (90 min)
        pm7t_add_event(event_log, patient_id, 7, current_time += 36000000000, 108, 60);   // Surgery prep (60 min)
        pm7t_add_event(event_log, patient_id, 8, current_time += 108000000000, 109, 180); // Surgery (3 hours)
        pm7t_add_event(event_log, patient_id, 9, current_time += 144000000000, 110, 240); // Post-op care (4 hours)
        pm7t_add_event(event_log, patient_id, 12, current_time += 72000000000, 111, 120); // Physical therapy (2 hours)
        pm7t_add_event(event_log, patient_id, 13, current_time += 36000000000, 112, 60);  // Discharge planning (60 min)
        pm7t_add_event(event_log, patient_id, 14, current_time += 18000000000, 113, 30);  // Discharge (30 min)
        pm7t_add_event(event_log, patient_id, 15, current_time += 27000000000, 114, 45);  // Follow-up (45 min)
        break;

      case 4:                                                                            // Outpatient procedure (50 instances)
        pm7t_add_event(event_log, patient_id, 0, current_time += 18000000000, 101, 30);  // Registration (30 min)
        pm7t_add_event(event_log, patient_id, 1, current_time += 27000000000, 102, 45);  // Triage (45 min)
        pm7t_add_event(event_log, patient_id, 5, current_time += 54000000000, 103, 90);  // Doctor consult (90 min)
        pm7t_add_event(event_log, patient_id, 3, current_time += 36000000000, 104, 60);  // Lab tests (60 min)
        pm7t_add_event(event_log, patient_id, 7, current_time += 18000000000, 105, 30);  // Procedure prep (30 min)
        pm7t_add_event(event_log, patient_id, 8, current_time += 72000000000, 106, 120); // Procedure (2 hours)
        pm7t_add_event(event_log, patient_id, 9, current_time += 36000000000, 107, 60);  // Post-procedure (60 min)
        pm7t_add_event(event_log, patient_id, 10, current_time += 27000000000, 108, 45); // Prescription (45 min)
        pm7t_add_event(event_log, patient_id, 11, current_time += 18000000000, 109, 30); // Pharmacy (30 min)
        pm7t_add_event(event_log, patient_id, 14, current_time += 15000000000, 110, 25); // Discharge (25 min)
        break;

      case 5:                                                                              // Insurance and billing intensive (50 instances)
        pm7t_add_event(event_log, patient_id, 0, current_time += 18000000000, 101, 30);    // Registration (30 min)
        pm7t_add_event(event_log, patient_id, 1, current_time += 27000000000, 102, 45);    // Triage (45 min)
        pm7t_add_event(event_log, patient_id, 5, current_time += 72000000000, 103, 120);   // Doctor consult (2 hours)
        pm7t_add_event(event_log, patient_id, 16, current_time += 54000000000, 104, 90);   // Insurance processing (90 min)
        pm7t_add_event(event_log, patient_id, 3, current_time += 36000000000, 105, 60);    // Lab tests (60 min)
        pm7t_add_event(event_log, patient_id, 17, current_time += 72000000000, 106, 120);  // Billing (2 hours)
        pm7t_add_event(event_log, patient_id, 18, current_time += 108000000000, 107, 180); // Payment collection (3 hours)
        pm7t_add_event(event_log, patient_id, 14, current_time += 18000000000, 108, 30);   // Discharge (30 min)
        break;

      case 6:                                                                             // Rehabilitation pathway (50 instances)
        pm7t_add_event(event_log, patient_id, 0, current_time += 18000000000, 101, 30);   // Registration (30 min)
        pm7t_add_event(event_log, patient_id, 1, current_time += 27000000000, 102, 45);   // Triage (45 min)
        pm7t_add_event(event_log, patient_id, 5, current_time += 72000000000, 103, 120);  // Doctor consult (2 hours)
        pm7t_add_event(event_log, patient_id, 6, current_time += 54000000000, 104, 90);   // Specialist referral (90 min)
        pm7t_add_event(event_log, patient_id, 12, current_time += 72000000000, 105, 120); // Physical therapy (2 hours)
        pm7t_add_event(event_log, patient_id, 12, current_time += 72000000000, 106, 120); // More PT (2 hours)
        pm7t_add_event(event_log, patient_id, 12, current_time += 72000000000, 107, 120); // Continued PT (2 hours)
        pm7t_add_event(event_log, patient_id, 13, current_time += 36000000000, 108, 60);  // Discharge planning (60 min)
        pm7t_add_event(event_log, patient_id, 14, current_time += 18000000000, 109, 30);  // Discharge (30 min)
        pm7t_add_event(event_log, patient_id, 15, current_time += 27000000000, 110, 45);  // Follow-up (45 min)
        break;

      case 7:                                                                           // Fast-track emergency (50 instances)
        pm7t_add_event(event_log, patient_id, 0, current_time += 900000000, 101, 15);   // Registration (9 min)
        pm7t_add_event(event_log, patient_id, 1, current_time += 1200000000, 102, 20);  // Triage (12 min)
        pm7t_add_event(event_log, patient_id, 2, current_time += 5400000000, 103, 90);  // Emergency care (54 min)
        pm7t_add_event(event_log, patient_id, 10, current_time += 1800000000, 104, 30); // Prescription (18 min)
        pm7t_add_event(event_log, patient_id, 11, current_time += 1200000000, 105, 20); // Pharmacy (12 min)
        pm7t_add_event(event_log, patient_id, 14, current_time += 900000000, 106, 15);  // Discharge (9 min)
        break;
      }

      patient_id++;
    }
  }

  printf("Generated %zu events across %u patient journeys\n",
         pm7t_get_event_count(event_log), patient_id - 1);
  printf("Unique activities: %u\n", pm7t_get_unique_activities(event_log));
  printf("Unique resources (staff): %u\n", pm7t_get_unique_resources(event_log));

  // Extract patient journey traces
  printf("\nExtracting patient journey traces...\n");
  TraceLog *trace_log = pm7t_extract_traces(event_log);
  if (!trace_log)
  {
    fprintf(stderr, "Failed to extract patient journey traces\n");
    pm7t_destroy_event_log(event_log);
    return 1;
  }

  printf("Extracted %zu patient journey traces\n", pm7t_get_trace_count(trace_log));

  // Display sample patient journeys
  printf("\nSample patient journeys:\n");
  for (size_t i = 0; i < 5 && i < pm7t_get_trace_count(trace_log); i++)
  {
    Trace *trace = pm7t_get_trace(trace_log, i);
    printf("Patient Journey %zu: ", i + 1);
    for (size_t j = 0; j < trace->size; j++)
    {
      printf("%s", activities[trace->activities[j]]);
      if (j < trace->size - 1)
        printf(" -> ");
    }
    printf("\n");
  }

  // Process discovery for healthcare workflows
  printf("\n=== Healthcare Process Discovery ===\n");

  // Alpha algorithm for healthcare processes
  printf("Discovering healthcare process model using Alpha algorithm...\n");
  ProcessModel *alpha_model = pm7t_discover_alpha_algorithm(trace_log);
  if (alpha_model)
  {
    printf("Alpha algorithm discovered %zu healthcare process transitions:\n", alpha_model->size);
    for (size_t i = 0; i < alpha_model->size; i++)
    {
      printf("  %s -> %s (freq: %u, prob: %.3f)\n",
             activities[alpha_model->transitions[i].from_activity],
             activities[alpha_model->transitions[i].to_activity],
             alpha_model->transitions[i].frequency,
             alpha_model->transitions[i].probability);
    }
  }

  // Heuristic miner for healthcare processes
  printf("\nDiscovering healthcare process model using Heuristic miner...\n");
  ProcessModel *heuristic_model = pm7t_discover_heuristic_miner(trace_log, 0.4);
  if (heuristic_model)
  {
    printf("Heuristic miner discovered %zu healthcare process transitions:\n", heuristic_model->size);
    for (size_t i = 0; i < heuristic_model->size; i++)
    {
      printf("  %s -> %s (freq: %u, prob: %.3f)\n",
             activities[heuristic_model->transitions[i].from_activity],
             activities[heuristic_model->transitions[i].to_activity],
             heuristic_model->transitions[i].frequency,
             heuristic_model->transitions[i].probability);
    }
  }

  // Healthcare process analysis
  printf("\n=== Healthcare Process Analysis ===\n");
  ProcessStats *process_stats = pm7t_analyze_process(event_log);
  if (process_stats)
  {
    printf("Healthcare activity statistics:\n");
    for (size_t i = 0; i < process_stats->size; i++)
    {
      ActivityStats *activity = &process_stats->activities[i];
      if (activity->frequency > 0)
      {
        printf("  %s: freq=%u, avg_duration=%.2f minutes\n",
               activities[activity->activity_id],
               activity->frequency,
               activity->avg_duration / 60000000000.0); // Convert to minutes
      }
    }
  }

  // Conformance checking for healthcare processes
  printf("\n=== Healthcare Process Conformance ===\n");
  if (alpha_model)
  {
    ConformanceResult alpha_conformance = pm7t_check_conformance(alpha_model, trace_log);
    printf("Alpha algorithm conformance for healthcare processes:\n");
    printf("  Fitness: %.3f (how well the model fits patient journeys)\n", alpha_conformance.fitness);
    printf("  Precision: %.3f (how precise the healthcare model is)\n", alpha_conformance.precision);
    printf("  Generalization: %.3f (how well the model generalizes)\n", alpha_conformance.generalization);
    printf("  Simplicity: %.3f (how simple the healthcare model is)\n", alpha_conformance.simplicity);
  }

  // Performance analysis for healthcare processes
  printf("\n=== Healthcare Performance Analysis ===\n");
  PerformanceAnalysis *performance = pm7t_analyze_performance(event_log);
  if (performance)
  {
    printf("Healthcare performance metrics:\n");
    printf("  Total patient journeys: %zu\n", performance->size);
    printf("  Average journey duration: %.2f hours\n",
           performance->avg_duration / 3600000000000.0); // Convert to hours
    printf("  Minimum journey duration: %.2f minutes\n",
           performance->min_duration / 60000000000.0); // Convert to minutes
    printf("  Maximum journey duration: %.2f hours\n",
           performance->max_duration / 3600000000000.0); // Convert to hours
    printf("  Patient throughput: %.2f patients/day\n",
           performance->throughput * 86400); // Convert to patients per day

    // Identify long-stay patients
    printf("\nLong-stay patients (journeys >4 hours):\n");
    int long_stay_count = 0;
    for (size_t i = 0; i < performance->size; i++)
    {
      double duration_hours = (double)performance->cases[i].duration / 3600000000000.0;
      if (duration_hours > 4.0)
      {
        printf("  Patient %u: %.2f hours, %u activities\n",
               performance->cases[i].case_id,
               duration_hours,
               performance->cases[i].num_activities);
        long_stay_count++;
      }
    }
    printf("  Total long-stay patients: %d (%.1f%%)\n",
           long_stay_count, (double)long_stay_count / performance->size * 100);
  }

  // Bottleneck analysis for healthcare processes
  printf("\n=== Healthcare Bottleneck Analysis ===\n");
  BottleneckAnalysis *bottlenecks = pm7t_analyze_bottlenecks(event_log);
  if (bottlenecks)
  {
    printf("Healthcare process bottlenecks:\n");
    for (size_t i = 0; i < bottlenecks->size; i++)
    {
      BottleneckInfo *bottleneck = &bottlenecks->bottlenecks[i];
      printf("  %s: avg_wait=%.2f min, avg_process=%.2f min, utilization=%.1f%%, queue=%u\n",
             activities[bottleneck->activity_id],
             bottleneck->avg_waiting_time / 60000000000.0,
             bottleneck->avg_processing_time / 60000000000.0,
             bottleneck->utilization * 100,
             bottleneck->queue_length);
    }
  }

  // Variant analysis for healthcare processes
  printf("\n=== Healthcare Journey Variants ===\n");
  VariantAnalysis *variants = pm7t_analyze_variants(trace_log);
  if (variants)
  {
    printf("Patient journey variants (top 10):\n");
    for (size_t i = 0; i < 10 && i < variants->size; i++)
    {
      Variant *variant = &variants->variants[i];
      printf("  Variant %zu: freq=%u (%.1f%%): ",
             i + 1, variant->frequency, variant->percentage);
      for (size_t j = 0; j < variant->trace->size; j++)
      {
        printf("%s", activities[variant->trace->activities[j]]);
        if (j < variant->trace->size - 1)
          printf(" -> ");
      }
      printf("\n");
    }
  }

  // Social network analysis for healthcare staff
  printf("\n=== Healthcare Staff Collaboration Network ===\n");
  SocialNetwork *network = pm7t_analyze_social_network(event_log);
  if (network)
  {
    printf("Healthcare staff collaboration patterns:\n");
    for (size_t i = 0; i < network->size; i++)
    {
      ResourceNode *resource = &network->resources[i];
      printf("  Staff %u: handovers=%u, centrality=%.3f\n",
             resource->resource_id,
             resource->handover_count,
             resource->centrality);
    }
  }

  // Cleanup
  if (network)
    pm7t_destroy_social_network(network);
  if (variants)
    pm7t_destroy_variant_analysis(variants);
  if (bottlenecks)
    pm7t_destroy_bottleneck_analysis(bottlenecks);
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

  printf("\nHealthcare process mining analysis completed successfully!\n");
  printf("Memory usage: %zu bytes\n", pm7t_get_memory_usage());

  return 0;
}