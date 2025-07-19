#include "../c_src/pm7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// IT Service Desk process mining demo
int main()
{
  printf("=== 7T IT Service Desk Process Mining Demo ===\n");
  printf("Analyzing support ticket lifecycles and incident management workflows\n\n");

  // Set memory limit (1.5GB for IT service data)
  pm7t_set_memory_limit(1536ULL * 1024 * 1024);

  // Create event log for IT service processes
  EventLog *event_log = pm7t_create_event_log(35000);
  if (!event_log)
  {
    fprintf(stderr, "Failed to create IT service event log\n");
    return 1;
  }

  // IT Service activity definitions
  const char *activities[] = {
      "Ticket Creation",
      "Initial Classification",
      "Priority Assignment",
      "First Level Support",
      "Issue Investigation",
      "Knowledge Base Search",
      "Escalation to Level 2",
      "Technical Analysis",
      "Escalation to Level 3",
      "Developer Investigation",
      "Code Review",
      "Bug Fix Development",
      "Testing and Validation",
      "Deployment Planning",
      "Change Management",
      "System Deployment",
      "User Acceptance Testing",
      "Ticket Resolution",
      "Customer Confirmation",
      "Ticket Closure",
      "Follow-up Survey",
      "Knowledge Base Update",
      "Process Documentation",
      "Root Cause Analysis",
      "Preventive Measures",
      "Training Schedule",
      "Equipment Replacement",
      "Software Installation",
      "Network Troubleshooting",
      "Security Incident Response",
      "Backup Restoration",
      "Data Recovery",
      "System Maintenance",
      "Performance Monitoring",
      "Capacity Planning",
      "Vendor Coordination",
      "License Management",
      "Asset Inventory Update",
      "Compliance Audit",
      "Service Level Review"};

  printf("Generating synthetic IT service desk process data...\n");

  // Generate synthetic IT service process data
  uint64_t base_time = time(NULL) * 1000000000ULL;
  uint32_t ticket_id = 1;
  srand(42); // For reproducible results

  // Generate different IT service scenarios
  for (int scenario = 0; scenario < 12; scenario++)
  {
    int instances = (scenario == 0) ? 80 : 40; // More instances for standard tickets

    for (int instance = 0; instance < instances; instance++)
    {
      uint64_t ticket_start = base_time + (ticket_id * 1000000);
      uint64_t current_time = ticket_start;

      switch (scenario)
      {
      case 0:                                                                     // Standard support ticket (80 instances)
        pm7t_add_event(event_log, ticket_id, 0, current_time += 1000, 101, 30);   // Ticket creation
        pm7t_add_event(event_log, ticket_id, 1, current_time += 2000, 102, 45);   // Initial classification
        pm7t_add_event(event_log, ticket_id, 2, current_time += 3000, 103, 30);   // Priority assignment
        pm7t_add_event(event_log, ticket_id, 3, current_time += 4000, 104, 120);  // First level support
        pm7t_add_event(event_log, ticket_id, 4, current_time += 5000, 105, 180);  // Issue investigation
        pm7t_add_event(event_log, ticket_id, 5, current_time += 6000, 106, 60);   // Knowledge base search
        pm7t_add_event(event_log, ticket_id, 17, current_time += 7000, 107, 90);  // Ticket resolution
        pm7t_add_event(event_log, ticket_id, 18, current_time += 8000, 108, 60);  // Customer confirmation
        pm7t_add_event(event_log, ticket_id, 19, current_time += 9000, 109, 30);  // Ticket closure
        pm7t_add_event(event_log, ticket_id, 20, current_time += 10000, 110, 45); // Follow-up survey
        break;

      case 1:                                                                     // Escalated ticket (40 instances)
        pm7t_add_event(event_log, ticket_id, 0, current_time += 1000, 101, 30);   // Ticket creation
        pm7t_add_event(event_log, ticket_id, 1, current_time += 2000, 102, 45);   // Initial classification
        pm7t_add_event(event_log, ticket_id, 2, current_time += 3000, 103, 30);   // Priority assignment
        pm7t_add_event(event_log, ticket_id, 3, current_time += 4000, 104, 120);  // First level support
        pm7t_add_event(event_log, ticket_id, 4, current_time += 5000, 105, 180);  // Issue investigation
        pm7t_add_event(event_log, ticket_id, 5, current_time += 6000, 106, 60);   // Knowledge base search
        pm7t_add_event(event_log, ticket_id, 6, current_time += 7000, 107, 90);   // Escalation to level 2
        pm7t_add_event(event_log, ticket_id, 7, current_time += 8000, 108, 240);  // Technical analysis
        pm7t_add_event(event_log, ticket_id, 17, current_time += 9000, 109, 90);  // Ticket resolution
        pm7t_add_event(event_log, ticket_id, 18, current_time += 10000, 110, 60); // Customer confirmation
        pm7t_add_event(event_log, ticket_id, 19, current_time += 11000, 111, 30); // Ticket closure
        pm7t_add_event(event_log, ticket_id, 20, current_time += 12000, 112, 45); // Follow-up survey
        break;

      case 2:                                                                      // Bug fix development (40 instances)
        pm7t_add_event(event_log, ticket_id, 0, current_time += 1000, 101, 30);    // Ticket creation
        pm7t_add_event(event_log, ticket_id, 1, current_time += 2000, 102, 45);    // Initial classification
        pm7t_add_event(event_log, ticket_id, 2, current_time += 3000, 103, 30);    // Priority assignment
        pm7t_add_event(event_log, ticket_id, 3, current_time += 4000, 104, 120);   // First level support
        pm7t_add_event(event_log, ticket_id, 4, current_time += 5000, 105, 180);   // Issue investigation
        pm7t_add_event(event_log, ticket_id, 6, current_time += 6000, 106, 90);    // Escalation to level 2
        pm7t_add_event(event_log, ticket_id, 7, current_time += 7000, 107, 240);   // Technical analysis
        pm7t_add_event(event_log, ticket_id, 8, current_time += 8000, 108, 120);   // Escalation to level 3
        pm7t_add_event(event_log, ticket_id, 9, current_time += 9000, 109, 180);   // Developer investigation
        pm7t_add_event(event_log, ticket_id, 10, current_time += 10000, 110, 240); // Code review
        pm7t_add_event(event_log, ticket_id, 11, current_time += 11000, 111, 360); // Bug fix development
        pm7t_add_event(event_log, ticket_id, 12, current_time += 12000, 112, 180); // Testing and validation
        pm7t_add_event(event_log, ticket_id, 13, current_time += 13000, 113, 120); // Deployment planning
        pm7t_add_event(event_log, ticket_id, 14, current_time += 14000, 114, 90);  // Change management
        pm7t_add_event(event_log, ticket_id, 15, current_time += 15000, 115, 60);  // System deployment
        pm7t_add_event(event_log, ticket_id, 16, current_time += 16000, 116, 120); // User acceptance testing
        pm7t_add_event(event_log, ticket_id, 17, current_time += 17000, 117, 90);  // Ticket resolution
        pm7t_add_event(event_log, ticket_id, 18, current_time += 18000, 118, 60);  // Customer confirmation
        pm7t_add_event(event_log, ticket_id, 19, current_time += 19000, 119, 30);  // Ticket closure
        break;

      case 3:                                                                      // Security incident (40 instances)
        pm7t_add_event(event_log, ticket_id, 0, current_time += 1000, 101, 30);    // Ticket creation
        pm7t_add_event(event_log, ticket_id, 1, current_time += 2000, 102, 45);    // Initial classification
        pm7t_add_event(event_log, ticket_id, 2, current_time += 3000, 103, 30);    // Priority assignment
        pm7t_add_event(event_log, ticket_id, 29, current_time += 4000, 104, 180);  // Security incident response
        pm7t_add_event(event_log, ticket_id, 4, current_time += 5000, 105, 240);   // Issue investigation
        pm7t_add_event(event_log, ticket_id, 6, current_time += 6000, 106, 90);    // Escalation to level 2
        pm7t_add_event(event_log, ticket_id, 7, current_time += 7000, 107, 300);   // Technical analysis
        pm7t_add_event(event_log, ticket_id, 8, current_time += 8000, 108, 120);   // Escalation to level 3
        pm7t_add_event(event_log, ticket_id, 9, current_time += 9000, 109, 360);   // Developer investigation
        pm7t_add_event(event_log, ticket_id, 23, current_time += 10000, 110, 180); // Root cause analysis
        pm7t_add_event(event_log, ticket_id, 24, current_time += 11000, 111, 240); // Preventive measures
        pm7t_add_event(event_log, ticket_id, 17, current_time += 12000, 112, 90);  // Ticket resolution
        pm7t_add_event(event_log, ticket_id, 18, current_time += 13000, 113, 60);  // Customer confirmation
        pm7t_add_event(event_log, ticket_id, 19, current_time += 14000, 114, 30);  // Ticket closure
        break;

      case 4:                                                                     // Data recovery (40 instances)
        pm7t_add_event(event_log, ticket_id, 0, current_time += 1000, 101, 30);   // Ticket creation
        pm7t_add_event(event_log, ticket_id, 1, current_time += 2000, 102, 45);   // Initial classification
        pm7t_add_event(event_log, ticket_id, 2, current_time += 3000, 103, 30);   // Priority assignment
        pm7t_add_event(event_log, ticket_id, 3, current_time += 4000, 104, 120);  // First level support
        pm7t_add_event(event_log, ticket_id, 4, current_time += 5000, 105, 180);  // Issue investigation
        pm7t_add_event(event_log, ticket_id, 30, current_time += 6000, 106, 240); // Backup restoration
        pm7t_add_event(event_log, ticket_id, 31, current_time += 7000, 107, 360); // Data recovery
        pm7t_add_event(event_log, ticket_id, 16, current_time += 8000, 108, 120); // User acceptance testing
        pm7t_add_event(event_log, ticket_id, 17, current_time += 9000, 109, 90);  // Ticket resolution
        pm7t_add_event(event_log, ticket_id, 18, current_time += 10000, 110, 60); // Customer confirmation
        pm7t_add_event(event_log, ticket_id, 19, current_time += 11000, 111, 30); // Ticket closure
        break;

      case 5:                                                                     // Equipment replacement (40 instances)
        pm7t_add_event(event_log, ticket_id, 0, current_time += 1000, 101, 30);   // Ticket creation
        pm7t_add_event(event_log, ticket_id, 1, current_time += 2000, 102, 45);   // Initial classification
        pm7t_add_event(event_log, ticket_id, 2, current_time += 3000, 103, 30);   // Priority assignment
        pm7t_add_event(event_log, ticket_id, 3, current_time += 4000, 104, 120);  // First level support
        pm7t_add_event(event_log, ticket_id, 4, current_time += 5000, 105, 180);  // Issue investigation
        pm7t_add_event(event_log, ticket_id, 26, current_time += 6000, 106, 120); // Equipment replacement
        pm7t_add_event(event_log, ticket_id, 27, current_time += 7000, 107, 180); // Software installation
        pm7t_add_event(event_log, ticket_id, 25, current_time += 8000, 108, 90);  // Training schedule
        pm7t_add_event(event_log, ticket_id, 17, current_time += 9000, 109, 90);  // Ticket resolution
        pm7t_add_event(event_log, ticket_id, 18, current_time += 10000, 110, 60); // Customer confirmation
        pm7t_add_event(event_log, ticket_id, 19, current_time += 11000, 111, 30); // Ticket closure
        pm7t_add_event(event_log, ticket_id, 37, current_time += 12000, 112, 60); // Asset inventory update
        break;

      case 6:                                                                     // Network troubleshooting (40 instances)
        pm7t_add_event(event_log, ticket_id, 0, current_time += 1000, 101, 30);   // Ticket creation
        pm7t_add_event(event_log, ticket_id, 1, current_time += 2000, 102, 45);   // Initial classification
        pm7t_add_event(event_log, ticket_id, 2, current_time += 3000, 103, 30);   // Priority assignment
        pm7t_add_event(event_log, ticket_id, 3, current_time += 4000, 104, 120);  // First level support
        pm7t_add_event(event_log, ticket_id, 4, current_time += 5000, 105, 180);  // Issue investigation
        pm7t_add_event(event_log, ticket_id, 28, current_time += 6000, 106, 240); // Network troubleshooting
        pm7t_add_event(event_log, ticket_id, 32, current_time += 7000, 107, 120); // System maintenance
        pm7t_add_event(event_log, ticket_id, 33, current_time += 8000, 108, 90);  // Performance monitoring
        pm7t_add_event(event_log, ticket_id, 17, current_time += 9000, 109, 90);  // Ticket resolution
        pm7t_add_event(event_log, ticket_id, 18, current_time += 10000, 110, 60); // Customer confirmation
        pm7t_add_event(event_log, ticket_id, 19, current_time += 11000, 111, 30); // Ticket closure
        break;

      case 7:                                                                     // Vendor coordination (40 instances)
        pm7t_add_event(event_log, ticket_id, 0, current_time += 1000, 101, 30);   // Ticket creation
        pm7t_add_event(event_log, ticket_id, 1, current_time += 2000, 102, 45);   // Initial classification
        pm7t_add_event(event_log, ticket_id, 2, current_time += 3000, 103, 30);   // Priority assignment
        pm7t_add_event(event_log, ticket_id, 3, current_time += 4000, 104, 120);  // First level support
        pm7t_add_event(event_log, ticket_id, 4, current_time += 5000, 105, 180);  // Issue investigation
        pm7t_add_event(event_log, ticket_id, 34, current_time += 6000, 106, 240); // Vendor coordination
        pm7t_add_event(event_log, ticket_id, 35, current_time += 7000, 107, 120); // License management
        pm7t_add_event(event_log, ticket_id, 17, current_time += 8000, 108, 90);  // Ticket resolution
        pm7t_add_event(event_log, ticket_id, 18, current_time += 9000, 109, 60);  // Customer confirmation
        pm7t_add_event(event_log, ticket_id, 19, current_time += 10000, 110, 30); // Ticket closure
        break;

      case 8:                                                                     // Capacity planning (40 instances)
        pm7t_add_event(event_log, ticket_id, 0, current_time += 1000, 101, 30);   // Ticket creation
        pm7t_add_event(event_log, ticket_id, 1, current_time += 2000, 102, 45);   // Initial classification
        pm7t_add_event(event_log, ticket_id, 2, current_time += 3000, 103, 30);   // Priority assignment
        pm7t_add_event(event_log, ticket_id, 3, current_time += 4000, 104, 120);  // First level support
        pm7t_add_event(event_log, ticket_id, 4, current_time += 5000, 105, 180);  // Issue investigation
        pm7t_add_event(event_log, ticket_id, 33, current_time += 6000, 106, 90);  // Performance monitoring
        pm7t_add_event(event_log, ticket_id, 34, current_time += 7000, 107, 360); // Capacity planning
        pm7t_add_event(event_log, ticket_id, 13, current_time += 8000, 108, 120); // Deployment planning
        pm7t_add_event(event_log, ticket_id, 14, current_time += 9000, 109, 90);  // Change management
        pm7t_add_event(event_log, ticket_id, 15, current_time += 10000, 110, 60); // System deployment
        pm7t_add_event(event_log, ticket_id, 17, current_time += 11000, 111, 90); // Ticket resolution
        pm7t_add_event(event_log, ticket_id, 18, current_time += 12000, 112, 60); // Customer confirmation
        pm7t_add_event(event_log, ticket_id, 19, current_time += 13000, 113, 30); // Ticket closure
        break;

      case 9:                                                                     // Compliance audit (40 instances)
        pm7t_add_event(event_log, ticket_id, 0, current_time += 1000, 101, 30);   // Ticket creation
        pm7t_add_event(event_log, ticket_id, 1, current_time += 2000, 102, 45);   // Initial classification
        pm7t_add_event(event_log, ticket_id, 2, current_time += 3000, 103, 30);   // Priority assignment
        pm7t_add_event(event_log, ticket_id, 3, current_time += 4000, 104, 120);  // First level support
        pm7t_add_event(event_log, ticket_id, 4, current_time += 5000, 105, 180);  // Issue investigation
        pm7t_add_event(event_log, ticket_id, 38, current_time += 6000, 106, 480); // Compliance audit
        pm7t_add_event(event_log, ticket_id, 22, current_time += 7000, 107, 180); // Process documentation
        pm7t_add_event(event_log, ticket_id, 21, current_time += 8000, 108, 120); // Knowledge base update
        pm7t_add_event(event_log, ticket_id, 17, current_time += 9000, 109, 90);  // Ticket resolution
        pm7t_add_event(event_log, ticket_id, 18, current_time += 10000, 110, 60); // Customer confirmation
        pm7t_add_event(event_log, ticket_id, 19, current_time += 11000, 111, 30); // Ticket closure
        break;

      case 10:                                                                    // Service level review (40 instances)
        pm7t_add_event(event_log, ticket_id, 0, current_time += 1000, 101, 30);   // Ticket creation
        pm7t_add_event(event_log, ticket_id, 1, current_time += 2000, 102, 45);   // Initial classification
        pm7t_add_event(event_log, ticket_id, 2, current_time += 3000, 103, 30);   // Priority assignment
        pm7t_add_event(event_log, ticket_id, 3, current_time += 4000, 104, 120);  // First level support
        pm7t_add_event(event_log, ticket_id, 4, current_time += 5000, 105, 180);  // Issue investigation
        pm7t_add_event(event_log, ticket_id, 39, current_time += 6000, 106, 240); // Service level review
        pm7t_add_event(event_log, ticket_id, 23, current_time += 7000, 107, 180); // Root cause analysis
        pm7t_add_event(event_log, ticket_id, 24, current_time += 8000, 108, 240); // Preventive measures
        pm7t_add_event(event_log, ticket_id, 21, current_time += 9000, 109, 120); // Knowledge base update
        pm7t_add_event(event_log, ticket_id, 17, current_time += 10000, 110, 90); // Ticket resolution
        pm7t_add_event(event_log, ticket_id, 18, current_time += 11000, 111, 60); // Customer confirmation
        pm7t_add_event(event_log, ticket_id, 19, current_time += 12000, 112, 30); // Ticket closure
        break;

      case 11:                                                                    // High-priority incident (40 instances)
        pm7t_add_event(event_log, ticket_id, 0, current_time += 500, 101, 15);    // Ticket creation
        pm7t_add_event(event_log, ticket_id, 1, current_time += 1000, 102, 22);   // Initial classification
        pm7t_add_event(event_log, ticket_id, 2, current_time += 1500, 103, 15);   // Priority assignment
        pm7t_add_event(event_log, ticket_id, 3, current_time += 2000, 104, 60);   // First level support
        pm7t_add_event(event_log, ticket_id, 4, current_time += 2500, 105, 90);   // Issue investigation
        pm7t_add_event(event_log, ticket_id, 6, current_time += 3000, 106, 45);   // Escalation to level 2
        pm7t_add_event(event_log, ticket_id, 7, current_time += 3500, 107, 120);  // Technical analysis
        pm7t_add_event(event_log, ticket_id, 8, current_time += 4000, 108, 60);   // Escalation to level 3
        pm7t_add_event(event_log, ticket_id, 9, current_time += 4500, 109, 90);   // Developer investigation
        pm7t_add_event(event_log, ticket_id, 11, current_time += 5000, 110, 180); // Bug fix development
        pm7t_add_event(event_log, ticket_id, 12, current_time += 5500, 111, 90);  // Testing and validation
        pm7t_add_event(event_log, ticket_id, 15, current_time += 6000, 112, 30);  // System deployment
        pm7t_add_event(event_log, ticket_id, 17, current_time += 6500, 113, 45);  // Ticket resolution
        pm7t_add_event(event_log, ticket_id, 18, current_time += 7000, 114, 30);  // Customer confirmation
        pm7t_add_event(event_log, ticket_id, 19, current_time += 7500, 115, 15);  // Ticket closure
        break;
      }

      ticket_id++;
    }
  }

  printf("Generated %zu events across %u IT service tickets\n",
         pm7t_get_event_count(event_log), ticket_id - 1);
  printf("Unique activities: %u\n", pm7t_get_unique_activities(event_log));
  printf("Unique resources (support staff): %u\n", pm7t_get_unique_resources(event_log));

  // Extract IT service process traces
  printf("\nExtracting IT service process traces...\n");
  TraceLog *trace_log = pm7t_extract_traces(event_log);
  if (!trace_log)
  {
    fprintf(stderr, "Failed to extract IT service process traces\n");
    pm7t_destroy_event_log(event_log);
    return 1;
  }

  printf("Extracted %zu IT service process traces\n", pm7t_get_trace_count(trace_log));

  // Display sample IT service processes
  printf("\nSample IT service processes:\n");
  for (size_t i = 0; i < 5 && i < pm7t_get_trace_count(trace_log); i++)
  {
    Trace *trace = pm7t_get_trace(trace_log, i);
    printf("IT Service Process %zu: ", i + 1);
    for (size_t j = 0; j < trace->size; j++)
    {
      printf("%s", activities[trace->activities[j]]);
      if (j < trace->size - 1)
        printf(" -> ");
    }
    printf("\n");
  }

  // Process discovery for IT service workflows
  printf("\n=== IT Service Process Discovery ===\n");

  // Alpha algorithm for IT service processes
  printf("Discovering IT service process model using Alpha algorithm...\n");
  ProcessModel *alpha_model = pm7t_discover_alpha_algorithm(trace_log);
  if (alpha_model)
  {
    printf("Alpha algorithm discovered %zu IT service process transitions:\n", alpha_model->size);
    for (size_t i = 0; i < alpha_model->size; i++)
    {
      printf("  %s -> %s (freq: %u, prob: %.3f)\n",
             activities[alpha_model->transitions[i].from_activity],
             activities[alpha_model->transitions[i].to_activity],
             alpha_model->transitions[i].frequency,
             alpha_model->transitions[i].probability);
    }
  }

  // Heuristic miner for IT service processes
  printf("\nDiscovering IT service process model using Heuristic miner...\n");
  ProcessModel *heuristic_model = pm7t_discover_heuristic_miner(trace_log, 0.3);
  if (heuristic_model)
  {
    printf("Heuristic miner discovered %zu IT service process transitions:\n", heuristic_model->size);
    for (size_t i = 0; i < heuristic_model->size; i++)
    {
      printf("  %s -> %s (freq: %u, prob: %.3f)\n",
             activities[heuristic_model->transitions[i].from_activity],
             activities[heuristic_model->transitions[i].to_activity],
             heuristic_model->transitions[i].frequency,
             heuristic_model->transitions[i].probability);
    }
  }

  // IT service process analysis
  printf("\n=== IT Service Process Analysis ===\n");
  ProcessStats *process_stats = pm7t_analyze_process(event_log);
  if (process_stats)
  {
    printf("IT service activity statistics:\n");
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

  // Conformance checking for IT service processes
  printf("\n=== IT Service Process Conformance ===\n");
  if (alpha_model)
  {
    ConformanceResult alpha_conformance = pm7t_check_conformance(alpha_model, trace_log);
    printf("Alpha algorithm conformance for IT service processes:\n");
    printf("  Fitness: %.3f (how well the model fits service processes)\n", alpha_conformance.fitness);
    printf("  Precision: %.3f (how precise the IT service model is)\n", alpha_conformance.precision);
    printf("  Generalization: %.3f (how well the model generalizes)\n", alpha_conformance.generalization);
    printf("  Simplicity: %.3f (how simple the IT service model is)\n", alpha_conformance.simplicity);
  }

  // Performance analysis for IT service processes
  printf("\n=== IT Service Performance Analysis ===\n");
  PerformanceAnalysis *performance = pm7t_analyze_performance(event_log);
  if (performance)
  {
    printf("IT service performance metrics:\n");
    printf("  Total tickets processed: %zu\n", performance->size);
    printf("  Average resolution time: %.2f hours\n",
           performance->avg_duration / 3600000000000.0); // Convert to hours
    printf("  Minimum resolution time: %.2f minutes\n",
           performance->min_duration / 60000000000.0); // Convert to minutes
    printf("  Maximum resolution time: %.2f hours\n",
           performance->max_duration / 3600000000000.0); // Convert to hours
    printf("  Ticket throughput: %.2f tickets/day\n",
           performance->throughput * 86400); // Convert to tickets per day

    // Identify long-running tickets
    printf("\nLong-running tickets (resolution >24 hours):\n");
    int long_ticket_count = 0;
    for (size_t i = 0; i < performance->size; i++)
    {
      double duration_hours = (double)performance->cases[i].duration / 3600000000000.0;
      if (duration_hours > 24.0)
      {
        printf("  Ticket %u: %.2f hours, %u activities\n",
               performance->cases[i].case_id,
               duration_hours,
               performance->cases[i].num_activities);
        long_ticket_count++;
      }
    }
    printf("  Total long-running tickets: %d (%.1f%%)\n",
           long_ticket_count, (double)long_ticket_count / performance->size * 100);
  }

  // Bottleneck analysis for IT service processes
  printf("\n=== IT Service Bottleneck Analysis ===\n");
  BottleneckAnalysis *bottlenecks = pm7t_analyze_bottlenecks(event_log);
  if (bottlenecks)
  {
    printf("IT service process bottlenecks:\n");
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

  // Variant analysis for IT service processes
  printf("\n=== IT Service Process Variants ===\n");
  VariantAnalysis *variants = pm7t_analyze_variants(trace_log);
  if (variants)
  {
    printf("IT service process variants (top 10):\n");
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

  // Social network analysis for IT service resources
  printf("\n=== IT Service Staff Collaboration Network ===\n");
  SocialNetwork *network = pm7t_analyze_social_network(event_log);
  if (network)
  {
    printf("IT service staff collaboration patterns:\n");
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

  printf("\nIT service process mining analysis completed successfully!\n");
  printf("Memory usage: %zu bytes\n", pm7t_get_memory_usage());

  return 0;
}