#include "../c_src/pm7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Financial process mining demo
int main()
{
  printf("=== 7T Financial Process Mining Demo ===\n");
  printf("Analyzing banking operations and transaction processing workflows\n\n");

  // Set memory limit (1.5GB for financial data)
  pm7t_set_memory_limit(1536ULL * 1024 * 1024);

  // Create event log for financial processes
  EventLog *event_log = pm7t_create_event_log(45000);
  if (!event_log)
  {
    fprintf(stderr, "Failed to create financial event log\n");
    return 1;
  }

  // Financial activity definitions
  const char *activities[] = {
      "Customer Onboarding",
      "Identity Verification",
      "KYC Processing",
      "Account Opening",
      "Credit Check",
      "Risk Assessment",
      "Account Activation",
      "Transaction Initiation",
      "Transaction Validation",
      "Fraud Detection",
      "Transaction Processing",
      "Settlement",
      "Reconciliation",
      "Statement Generation",
      "Customer Notification",
      "Loan Application",
      "Loan Underwriting",
      "Loan Approval",
      "Loan Disbursement",
      "Payment Processing",
      "Payment Collection",
      "Default Management",
      "Collections Process",
      "Account Closure",
      "Compliance Review",
      "Regulatory Reporting",
      "Audit Trail",
      "Security Review",
      "System Maintenance",
      "Backup Processing",
      "Data Archival",
      "Customer Support",
      "Dispute Resolution",
      "Refund Processing",
      "Interest Calculation",
      "Fee Assessment",
      "Tax Processing",
      "Investment Processing",
      "Portfolio Management",
      "Market Analysis",
      "Trading Execution",
      "Order Management",
      "Settlement Confirmation"};

  printf("Generating synthetic financial process data...\n");

  // Generate synthetic financial process data
  uint64_t base_time = time(NULL) * 1000000000ULL;
  uint32_t transaction_id = 1;
  srand(42); // For reproducible results

  // Generate different financial scenarios
  for (int scenario = 0; scenario < 10; scenario++)
  {
    int instances = (scenario == 0) ? 80 : 40; // More instances for standard transactions

    for (int instance = 0; instance < instances; instance++)
    {
      uint64_t process_start = base_time + (transaction_id * 1000000);
      uint64_t current_time = process_start;

      switch (scenario)
      {
      case 0:                                                                          // Standard banking transaction (80 instances)
        pm7t_add_event(event_log, transaction_id, 0, current_time += 1000, 101, 120);  // Customer onboarding
        pm7t_add_event(event_log, transaction_id, 1, current_time += 2000, 102, 90);   // Identity verification
        pm7t_add_event(event_log, transaction_id, 2, current_time += 3000, 103, 180);  // KYC processing
        pm7t_add_event(event_log, transaction_id, 3, current_time += 4000, 104, 60);   // Account opening
        pm7t_add_event(event_log, transaction_id, 4, current_time += 5000, 105, 120);  // Credit check
        pm7t_add_event(event_log, transaction_id, 5, current_time += 6000, 106, 90);   // Risk assessment
        pm7t_add_event(event_log, transaction_id, 6, current_time += 7000, 107, 30);   // Account activation
        pm7t_add_event(event_log, transaction_id, 7, current_time += 8000, 108, 60);   // Transaction initiation
        pm7t_add_event(event_log, transaction_id, 8, current_time += 9000, 109, 45);   // Transaction validation
        pm7t_add_event(event_log, transaction_id, 9, current_time += 10000, 110, 30);  // Fraud detection
        pm7t_add_event(event_log, transaction_id, 10, current_time += 11000, 111, 90); // Transaction processing
        pm7t_add_event(event_log, transaction_id, 11, current_time += 12000, 112, 60); // Settlement
        pm7t_add_event(event_log, transaction_id, 12, current_time += 13000, 113, 45); // Reconciliation
        pm7t_add_event(event_log, transaction_id, 13, current_time += 14000, 114, 30); // Statement generation
        pm7t_add_event(event_log, transaction_id, 14, current_time += 15000, 115, 15); // Customer notification
        break;

      case 1:                                                                          // Loan application process (40 instances)
        pm7t_add_event(event_log, transaction_id, 0, current_time += 1000, 101, 120);  // Customer onboarding
        pm7t_add_event(event_log, transaction_id, 1, current_time += 2000, 102, 90);   // Identity verification
        pm7t_add_event(event_log, transaction_id, 2, current_time += 3000, 103, 180);  // KYC processing
        pm7t_add_event(event_log, transaction_id, 15, current_time += 4000, 104, 240); // Loan application
        pm7t_add_event(event_log, transaction_id, 4, current_time += 5000, 105, 120);  // Credit check
        pm7t_add_event(event_log, transaction_id, 5, current_time += 6000, 106, 90);   // Risk assessment
        pm7t_add_event(event_log, transaction_id, 16, current_time += 7000, 107, 360); // Loan underwriting
        pm7t_add_event(event_log, transaction_id, 17, current_time += 8000, 108, 180); // Loan approval
        pm7t_add_event(event_log, transaction_id, 18, current_time += 9000, 109, 120); // Loan disbursement
        pm7t_add_event(event_log, transaction_id, 19, current_time += 10000, 110, 90); // Payment processing
        pm7t_add_event(event_log, transaction_id, 20, current_time += 11000, 111, 60); // Payment collection
        break;

      case 2:                                                                           // Fraud detection scenario (40 instances)
        pm7t_add_event(event_log, transaction_id, 0, current_time += 1000, 101, 120);   // Customer onboarding
        pm7t_add_event(event_log, transaction_id, 1, current_time += 2000, 102, 90);    // Identity verification
        pm7t_add_event(event_log, transaction_id, 2, current_time += 3000, 103, 180);   // KYC processing
        pm7t_add_event(event_log, transaction_id, 3, current_time += 4000, 104, 60);    // Account opening
        pm7t_add_event(event_log, transaction_id, 4, current_time += 5000, 105, 120);   // Credit check
        pm7t_add_event(event_log, transaction_id, 5, current_time += 6000, 106, 90);    // Risk assessment
        pm7t_add_event(event_log, transaction_id, 6, current_time += 7000, 107, 30);    // Account activation
        pm7t_add_event(event_log, transaction_id, 7, current_time += 8000, 108, 60);    // Transaction initiation
        pm7t_add_event(event_log, transaction_id, 8, current_time += 9000, 109, 45);    // Transaction validation
        pm7t_add_event(event_log, transaction_id, 9, current_time += 10000, 110, 30);   // Fraud detection
        pm7t_add_event(event_log, transaction_id, 27, current_time += 11000, 111, 180); // Audit trail
        pm7t_add_event(event_log, transaction_id, 28, current_time += 12000, 112, 120); // Security review
        pm7t_add_event(event_log, transaction_id, 31, current_time += 13000, 113, 240); // Customer support
        pm7t_add_event(event_log, transaction_id, 32, current_time += 14000, 114, 180); // Dispute resolution
        break;

      case 3:                                                                          // Compliance and regulatory process (40 instances)
        pm7t_add_event(event_log, transaction_id, 0, current_time += 1000, 101, 120);  // Customer onboarding
        pm7t_add_event(event_log, transaction_id, 1, current_time += 2000, 102, 90);   // Identity verification
        pm7t_add_event(event_log, transaction_id, 2, current_time += 3000, 103, 180);  // KYC processing
        pm7t_add_event(event_log, transaction_id, 24, current_time += 4000, 104, 240); // Compliance review
        pm7t_add_event(event_log, transaction_id, 25, current_time += 5000, 105, 360); // Regulatory reporting
        pm7t_add_event(event_log, transaction_id, 27, current_time += 6000, 106, 180); // Audit trail
        pm7t_add_event(event_log, transaction_id, 3, current_time += 7000, 107, 60);   // Account opening
        pm7t_add_event(event_log, transaction_id, 4, current_time += 8000, 108, 120);  // Credit check
        pm7t_add_event(event_log, transaction_id, 5, current_time += 9000, 109, 90);   // Risk assessment
        pm7t_add_event(event_log, transaction_id, 6, current_time += 10000, 110, 30);  // Account activation
        pm7t_add_event(event_log, transaction_id, 7, current_time += 11000, 111, 60);  // Transaction initiation
        pm7t_add_event(event_log, transaction_id, 8, current_time += 12000, 112, 45);  // Transaction validation
        pm7t_add_event(event_log, transaction_id, 9, current_time += 13000, 113, 30);  // Fraud detection
        pm7t_add_event(event_log, transaction_id, 10, current_time += 14000, 114, 90); // Transaction processing
        pm7t_add_event(event_log, transaction_id, 11, current_time += 15000, 115, 60); // Settlement
        break;

      case 4:                                                                           // Default and collections process (40 instances)
        pm7t_add_event(event_log, transaction_id, 0, current_time += 1000, 101, 120);   // Customer onboarding
        pm7t_add_event(event_log, transaction_id, 1, current_time += 2000, 102, 90);    // Identity verification
        pm7t_add_event(event_log, transaction_id, 2, current_time += 3000, 103, 180);   // KYC processing
        pm7t_add_event(event_log, transaction_id, 15, current_time += 4000, 104, 240);  // Loan application
        pm7t_add_event(event_log, transaction_id, 4, current_time += 5000, 105, 120);   // Credit check
        pm7t_add_event(event_log, transaction_id, 5, current_time += 6000, 106, 90);    // Risk assessment
        pm7t_add_event(event_log, transaction_id, 16, current_time += 7000, 107, 360);  // Loan underwriting
        pm7t_add_event(event_log, transaction_id, 17, current_time += 8000, 108, 180);  // Loan approval
        pm7t_add_event(event_log, transaction_id, 18, current_time += 9000, 109, 120);  // Loan disbursement
        pm7t_add_event(event_log, transaction_id, 19, current_time += 10000, 110, 90);  // Payment processing
        pm7t_add_event(event_log, transaction_id, 21, current_time += 11000, 111, 180); // Default management
        pm7t_add_event(event_log, transaction_id, 22, current_time += 12000, 112, 240); // Collections process
        break;

      case 5:                                                                          // Investment and trading process (40 instances)
        pm7t_add_event(event_log, transaction_id, 0, current_time += 1000, 101, 120);  // Customer onboarding
        pm7t_add_event(event_log, transaction_id, 1, current_time += 2000, 102, 90);   // Identity verification
        pm7t_add_event(event_log, transaction_id, 2, current_time += 3000, 103, 180);  // KYC processing
        pm7t_add_event(event_log, transaction_id, 37, current_time += 4000, 104, 240); // Investment processing
        pm7t_add_event(event_log, transaction_id, 38, current_time += 5000, 105, 180); // Portfolio management
        pm7t_add_event(event_log, transaction_id, 39, current_time += 6000, 106, 120); // Market analysis
        pm7t_add_event(event_log, transaction_id, 40, current_time += 7000, 107, 60);  // Trading execution
        pm7t_add_event(event_log, transaction_id, 41, current_time += 8000, 108, 90);  // Order management
        pm7t_add_event(event_log, transaction_id, 10, current_time += 9000, 109, 90);  // Transaction processing
        pm7t_add_event(event_log, transaction_id, 11, current_time += 10000, 110, 60); // Settlement
        pm7t_add_event(event_log, transaction_id, 42, current_time += 11000, 111, 45); // Settlement confirmation
        pm7t_add_event(event_log, transaction_id, 12, current_time += 12000, 112, 45); // Reconciliation
        pm7t_add_event(event_log, transaction_id, 13, current_time += 13000, 113, 30); // Statement generation
        pm7t_add_event(event_log, transaction_id, 14, current_time += 14000, 114, 15); // Customer notification
        break;

      case 6:                                                                          // System maintenance and backup (40 instances)
        pm7t_add_event(event_log, transaction_id, 0, current_time += 1000, 101, 120);  // Customer onboarding
        pm7t_add_event(event_log, transaction_id, 1, current_time += 2000, 102, 90);   // Identity verification
        pm7t_add_event(event_log, transaction_id, 2, current_time += 3000, 103, 180);  // KYC processing
        pm7t_add_event(event_log, transaction_id, 28, current_time += 4000, 104, 240); // System maintenance
        pm7t_add_event(event_log, transaction_id, 29, current_time += 5000, 105, 180); // Backup processing
        pm7t_add_event(event_log, transaction_id, 30, current_time += 6000, 106, 120); // Data archival
        pm7t_add_event(event_log, transaction_id, 3, current_time += 7000, 107, 60);   // Account opening
        pm7t_add_event(event_log, transaction_id, 4, current_time += 8000, 108, 120);  // Credit check
        pm7t_add_event(event_log, transaction_id, 5, current_time += 9000, 109, 90);   // Risk assessment
        pm7t_add_event(event_log, transaction_id, 6, current_time += 10000, 110, 30);  // Account activation
        pm7t_add_event(event_log, transaction_id, 7, current_time += 11000, 111, 60);  // Transaction initiation
        pm7t_add_event(event_log, transaction_id, 8, current_time += 12000, 112, 45);  // Transaction validation
        pm7t_add_event(event_log, transaction_id, 9, current_time += 13000, 113, 30);  // Fraud detection
        pm7t_add_event(event_log, transaction_id, 10, current_time += 14000, 114, 90); // Transaction processing
        pm7t_add_event(event_log, transaction_id, 11, current_time += 15000, 115, 60); // Settlement
        break;

      case 7:                                                                           // Refund and dispute resolution (40 instances)
        pm7t_add_event(event_log, transaction_id, 0, current_time += 1000, 101, 120);   // Customer onboarding
        pm7t_add_event(event_log, transaction_id, 1, current_time += 2000, 102, 90);    // Identity verification
        pm7t_add_event(event_log, transaction_id, 2, current_time += 3000, 103, 180);   // KYC processing
        pm7t_add_event(event_log, transaction_id, 3, current_time += 4000, 104, 60);    // Account opening
        pm7t_add_event(event_log, transaction_id, 4, current_time += 5000, 105, 120);   // Credit check
        pm7t_add_event(event_log, transaction_id, 5, current_time += 6000, 106, 90);    // Risk assessment
        pm7t_add_event(event_log, transaction_id, 6, current_time += 7000, 107, 30);    // Account activation
        pm7t_add_event(event_log, transaction_id, 7, current_time += 8000, 108, 60);    // Transaction initiation
        pm7t_add_event(event_log, transaction_id, 8, current_time += 9000, 109, 45);    // Transaction validation
        pm7t_add_event(event_log, transaction_id, 9, current_time += 10000, 110, 30);   // Fraud detection
        pm7t_add_event(event_log, transaction_id, 10, current_time += 11000, 111, 90);  // Transaction processing
        pm7t_add_event(event_log, transaction_id, 11, current_time += 12000, 112, 60);  // Settlement
        pm7t_add_event(event_log, transaction_id, 31, current_time += 13000, 113, 180); // Customer support
        pm7t_add_event(event_log, transaction_id, 32, current_time += 14000, 114, 240); // Dispute resolution
        pm7t_add_event(event_log, transaction_id, 33, current_time += 15000, 115, 120); // Refund processing
        break;

      case 8:                                                                          // High-frequency trading (40 instances)
        pm7t_add_event(event_log, transaction_id, 0, current_time += 500, 101, 60);    // Customer onboarding
        pm7t_add_event(event_log, transaction_id, 1, current_time += 1000, 102, 45);   // Identity verification
        pm7t_add_event(event_log, transaction_id, 2, current_time += 1500, 103, 90);   // KYC processing
        pm7t_add_event(event_log, transaction_id, 37, current_time += 2000, 104, 120); // Investment processing
        pm7t_add_event(event_log, transaction_id, 38, current_time += 2500, 105, 90);  // Portfolio management
        pm7t_add_event(event_log, transaction_id, 39, current_time += 3000, 106, 60);  // Market analysis
        pm7t_add_event(event_log, transaction_id, 40, current_time += 3500, 107, 30);  // Trading execution
        pm7t_add_event(event_log, transaction_id, 41, current_time += 4000, 108, 45);  // Order management
        pm7t_add_event(event_log, transaction_id, 10, current_time += 4500, 109, 45);  // Transaction processing
        pm7t_add_event(event_log, transaction_id, 11, current_time += 5000, 110, 30);  // Settlement
        pm7t_add_event(event_log, transaction_id, 42, current_time += 5500, 111, 22);  // Settlement confirmation
        break;

      case 9:                                                                           // Account closure process (40 instances)
        pm7t_add_event(event_log, transaction_id, 0, current_time += 1000, 101, 120);   // Customer onboarding
        pm7t_add_event(event_log, transaction_id, 1, current_time += 2000, 102, 90);    // Identity verification
        pm7t_add_event(event_log, transaction_id, 2, current_time += 3000, 103, 180);   // KYC processing
        pm7t_add_event(event_log, transaction_id, 3, current_time += 4000, 104, 60);    // Account opening
        pm7t_add_event(event_log, transaction_id, 4, current_time += 5000, 105, 120);   // Credit check
        pm7t_add_event(event_log, transaction_id, 5, current_time += 6000, 106, 90);    // Risk assessment
        pm7t_add_event(event_log, transaction_id, 6, current_time += 7000, 107, 30);    // Account activation
        pm7t_add_event(event_log, transaction_id, 7, current_time += 8000, 108, 60);    // Transaction initiation
        pm7t_add_event(event_log, transaction_id, 8, current_time += 9000, 109, 45);    // Transaction validation
        pm7t_add_event(event_log, transaction_id, 9, current_time += 10000, 110, 30);   // Fraud detection
        pm7t_add_event(event_log, transaction_id, 10, current_time += 11000, 111, 90);  // Transaction processing
        pm7t_add_event(event_log, transaction_id, 11, current_time += 12000, 112, 60);  // Settlement
        pm7t_add_event(event_log, transaction_id, 12, current_time += 13000, 113, 45);  // Reconciliation
        pm7t_add_event(event_log, transaction_id, 23, current_time += 14000, 114, 180); // Account closure
        break;
      }

      transaction_id++;
    }
  }

  printf("Generated %zu events across %u financial processes\n",
         pm7t_get_event_count(event_log), transaction_id - 1);
  printf("Unique activities: %u\n", pm7t_get_unique_activities(event_log));
  printf("Unique resources (staff/systems): %u\n", pm7t_get_unique_resources(event_log));

  // Extract financial process traces
  printf("\nExtracting financial process traces...\n");
  TraceLog *trace_log = pm7t_extract_traces(event_log);
  if (!trace_log)
  {
    fprintf(stderr, "Failed to extract financial process traces\n");
    pm7t_destroy_event_log(event_log);
    return 1;
  }

  printf("Extracted %zu financial process traces\n", pm7t_get_trace_count(trace_log));

  // Display sample financial processes
  printf("\nSample financial processes:\n");
  for (size_t i = 0; i < 5 && i < pm7t_get_trace_count(trace_log); i++)
  {
    Trace *trace = pm7t_get_trace(trace_log, i);
    printf("Financial Process %zu: ", i + 1);
    for (size_t j = 0; j < trace->size; j++)
    {
      printf("%s", activities[trace->activities[j]]);
      if (j < trace->size - 1)
        printf(" -> ");
    }
    printf("\n");
  }

  // Process discovery for financial workflows
  printf("\n=== Financial Process Discovery ===\n");

  // Alpha algorithm for financial processes
  printf("Discovering financial process model using Alpha algorithm...\n");
  ProcessModel *alpha_model = pm7t_discover_alpha_algorithm(trace_log);
  if (alpha_model)
  {
    printf("Alpha algorithm discovered %zu financial process transitions:\n", alpha_model->size);
    for (size_t i = 0; i < alpha_model->size; i++)
    {
      printf("  %s -> %s (freq: %u, prob: %.3f)\n",
             activities[alpha_model->transitions[i].from_activity],
             activities[alpha_model->transitions[i].to_activity],
             alpha_model->transitions[i].frequency,
             alpha_model->transitions[i].probability);
    }
  }

  // Heuristic miner for financial processes
  printf("\nDiscovering financial process model using Heuristic miner...\n");
  ProcessModel *heuristic_model = pm7t_discover_heuristic_miner(trace_log, 0.3);
  if (heuristic_model)
  {
    printf("Heuristic miner discovered %zu financial process transitions:\n", heuristic_model->size);
    for (size_t i = 0; i < heuristic_model->size; i++)
    {
      printf("  %s -> %s (freq: %u, prob: %.3f)\n",
             activities[heuristic_model->transitions[i].from_activity],
             activities[heuristic_model->transitions[i].to_activity],
             heuristic_model->transitions[i].frequency,
             heuristic_model->transitions[i].probability);
    }
  }

  // Financial process analysis
  printf("\n=== Financial Process Analysis ===\n");
  ProcessStats *process_stats = pm7t_analyze_process(event_log);
  if (process_stats)
  {
    printf("Financial activity statistics:\n");
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

  // Conformance checking for financial processes
  printf("\n=== Financial Process Conformance ===\n");
  if (alpha_model)
  {
    ConformanceResult alpha_conformance = pm7t_check_conformance(alpha_model, trace_log);
    printf("Alpha algorithm conformance for financial processes:\n");
    printf("  Fitness: %.3f (how well the model fits financial processes)\n", alpha_conformance.fitness);
    printf("  Precision: %.3f (how precise the financial model is)\n", alpha_conformance.precision);
    printf("  Generalization: %.3f (how well the model generalizes)\n", alpha_conformance.generalization);
    printf("  Simplicity: %.3f (how simple the financial model is)\n", alpha_conformance.simplicity);
  }

  // Performance analysis for financial processes
  printf("\n=== Financial Performance Analysis ===\n");
  PerformanceAnalysis *performance = pm7t_analyze_performance(event_log);
  if (performance)
  {
    printf("Financial performance metrics:\n");
    printf("  Total financial processes: %zu\n", performance->size);
    printf("  Average process time: %.2f hours\n",
           performance->avg_duration / 3600000000000.0); // Convert to hours
    printf("  Minimum process time: %.2f minutes\n",
           performance->min_duration / 60000000000.0); // Convert to minutes
    printf("  Maximum process time: %.2f hours\n",
           performance->max_duration / 3600000000000.0); // Convert to hours
    printf("  Process throughput: %.2f processes/day\n",
           performance->throughput * 86400); // Convert to processes per day

    // Identify slow processes
    printf("\nSlow processes (processing >2 hours):\n");
    int slow_process_count = 0;
    for (size_t i = 0; i < performance->size; i++)
    {
      double duration_hours = (double)performance->cases[i].duration / 3600000000000.0;
      if (duration_hours > 2.0)
      {
        printf("  Process %u: %.2f hours, %u activities\n",
               performance->cases[i].case_id,
               duration_hours,
               performance->cases[i].num_activities);
        slow_process_count++;
      }
    }
    printf("  Total slow processes: %d (%.1f%%)\n",
           slow_process_count, (double)slow_process_count / performance->size * 100);
  }

  // Bottleneck analysis for financial processes
  printf("\n=== Financial Bottleneck Analysis ===\n");
  BottleneckAnalysis *bottlenecks = pm7t_analyze_bottlenecks(event_log);
  if (bottlenecks)
  {
    printf("Financial process bottlenecks:\n");
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

  // Variant analysis for financial processes
  printf("\n=== Financial Process Variants ===\n");
  VariantAnalysis *variants = pm7t_analyze_variants(trace_log);
  if (variants)
  {
    printf("Financial process variants (top 10):\n");
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

  // Social network analysis for financial resources
  printf("\n=== Financial Resource Collaboration Network ===\n");
  SocialNetwork *network = pm7t_analyze_social_network(event_log);
  if (network)
  {
    printf("Financial resource collaboration patterns:\n");
    for (size_t i = 0; i < network->size; i++)
    {
      ResourceNode *resource = &network->resources[i];
      printf("  Resource %u: handovers=%u, centrality=%.3f\n",
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

  printf("\nFinancial process mining analysis completed successfully!\n");
  printf("Memory usage: %zu bytes\n", pm7t_get_memory_usage());

  return 0;
}