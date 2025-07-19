#include "../c_src/pm7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// E-commerce process mining demo
int main()
{
  printf("=== 7T E-commerce Process Mining Demo ===\n");
  printf("Analyzing order fulfillment and customer service workflows\n\n");

  // Set memory limit (1.5GB for e-commerce data)
  pm7t_set_memory_limit(1536ULL * 1024 * 1024);

  // Create event log for e-commerce processes
  EventLog *event_log = pm7t_create_event_log(40000);
  if (!event_log)
  {
    fprintf(stderr, "Failed to create e-commerce event log\n");
    return 1;
  }

  // E-commerce activity definitions
  const char *activities[] = {
      "Order Received",
      "Payment Processing",
      "Payment Verification",
      "Inventory Check",
      "Order Confirmation",
      "Pick and Pack",
      "Quality Check",
      "Shipping Label Generation",
      "Package Handoff to Carrier",
      "In Transit",
      "Out for Delivery",
      "Delivered",
      "Customer Feedback",
      "Return Request",
      "Return Processing",
      "Refund Processing",
      "Customer Support Ticket",
      "Issue Resolution",
      "Order Cancellation",
      "Inventory Restock",
      "Supplier Order",
      "Supplier Delivery",
      "Warehouse Receiving",
      "Product Catalog Update"};

  printf("Generating synthetic e-commerce process data...\n");

  // Generate synthetic e-commerce process data
  uint64_t base_time = time(NULL) * 1000000000ULL;
  uint32_t order_id = 1;
  srand(42); // For reproducible results

  // Generate different order fulfillment scenarios
  for (int scenario = 0; scenario < 10; scenario++)
  {
    int instances = (scenario == 0) ? 80 : 40; // More instances for standard orders

    for (int instance = 0; instance < instances; instance++)
    {
      uint64_t order_start = base_time + (order_id * 1000000);
      uint64_t current_time = order_start;

      switch (scenario)
      {
      case 0:                                                                     // Standard order fulfillment (80 instances)
        pm7t_add_event(event_log, order_id, 0, current_time += 1000, 101, 30);    // Order received
        pm7t_add_event(event_log, order_id, 1, current_time += 2000, 102, 45);    // Payment processing
        pm7t_add_event(event_log, order_id, 2, current_time += 3000, 103, 30);    // Payment verification
        pm7t_add_event(event_log, order_id, 3, current_time += 4000, 104, 60);    // Inventory check
        pm7t_add_event(event_log, order_id, 4, current_time += 5000, 105, 30);    // Order confirmation
        pm7t_add_event(event_log, order_id, 5, current_time += 6000, 106, 120);   // Pick and pack
        pm7t_add_event(event_log, order_id, 6, current_time += 7000, 107, 60);    // Quality check
        pm7t_add_event(event_log, order_id, 7, current_time += 8000, 108, 45);    // Shipping label
        pm7t_add_event(event_log, order_id, 8, current_time += 9000, 109, 30);    // Handoff to carrier
        pm7t_add_event(event_log, order_id, 9, current_time += 10000, 110, 1800); // In transit
        pm7t_add_event(event_log, order_id, 10, current_time += 11000, 111, 300); // Out for delivery
        pm7t_add_event(event_log, order_id, 11, current_time += 12000, 112, 60);  // Delivered
        pm7t_add_event(event_log, order_id, 12, current_time += 13000, 113, 90);  // Customer feedback
        break;

      case 1:                                                                     // Order with customer support (40 instances)
        pm7t_add_event(event_log, order_id, 0, current_time += 1000, 101, 30);    // Order received
        pm7t_add_event(event_log, order_id, 1, current_time += 2000, 102, 45);    // Payment processing
        pm7t_add_event(event_log, order_id, 2, current_time += 3000, 103, 30);    // Payment verification
        pm7t_add_event(event_log, order_id, 3, current_time += 4000, 104, 60);    // Inventory check
        pm7t_add_event(event_log, order_id, 4, current_time += 5000, 105, 30);    // Order confirmation
        pm7t_add_event(event_log, order_id, 16, current_time += 6000, 106, 120);  // Support ticket
        pm7t_add_event(event_log, order_id, 17, current_time += 7000, 107, 240);  // Issue resolution
        pm7t_add_event(event_log, order_id, 5, current_time += 8000, 108, 120);   // Pick and pack
        pm7t_add_event(event_log, order_id, 6, current_time += 9000, 109, 60);    // Quality check
        pm7t_add_event(event_log, order_id, 7, current_time += 10000, 110, 45);   // Shipping label
        pm7t_add_event(event_log, order_id, 8, current_time += 11000, 111, 30);   // Handoff to carrier
        pm7t_add_event(event_log, order_id, 9, current_time += 12000, 112, 1800); // In transit
        pm7t_add_event(event_log, order_id, 10, current_time += 13000, 113, 300); // Out for delivery
        pm7t_add_event(event_log, order_id, 11, current_time += 14000, 114, 60);  // Delivered
        break;

      case 2:                                                                     // Order with return processing (40 instances)
        pm7t_add_event(event_log, order_id, 0, current_time += 1000, 101, 30);    // Order received
        pm7t_add_event(event_log, order_id, 1, current_time += 2000, 102, 45);    // Payment processing
        pm7t_add_event(event_log, order_id, 2, current_time += 3000, 103, 30);    // Payment verification
        pm7t_add_event(event_log, order_id, 3, current_time += 4000, 104, 60);    // Inventory check
        pm7t_add_event(event_log, order_id, 4, current_time += 5000, 105, 30);    // Order confirmation
        pm7t_add_event(event_log, order_id, 5, current_time += 6000, 106, 120);   // Pick and pack
        pm7t_add_event(event_log, order_id, 6, current_time += 7000, 107, 60);    // Quality check
        pm7t_add_event(event_log, order_id, 7, current_time += 8000, 108, 45);    // Shipping label
        pm7t_add_event(event_log, order_id, 8, current_time += 9000, 109, 30);    // Handoff to carrier
        pm7t_add_event(event_log, order_id, 9, current_time += 10000, 110, 1800); // In transit
        pm7t_add_event(event_log, order_id, 10, current_time += 11000, 111, 300); // Out for delivery
        pm7t_add_event(event_log, order_id, 11, current_time += 12000, 112, 60);  // Delivered
        pm7t_add_event(event_log, order_id, 13, current_time += 13000, 113, 90);  // Return request
        pm7t_add_event(event_log, order_id, 14, current_time += 14000, 114, 180); // Return processing
        pm7t_add_event(event_log, order_id, 15, current_time += 15000, 115, 120); // Refund processing
        pm7t_add_event(event_log, order_id, 19, current_time += 16000, 116, 60);  // Inventory restock
        break;

      case 3:                                                                    // Order cancellation (40 instances)
        pm7t_add_event(event_log, order_id, 0, current_time += 1000, 101, 30);   // Order received
        pm7t_add_event(event_log, order_id, 1, current_time += 2000, 102, 45);   // Payment processing
        pm7t_add_event(event_log, order_id, 2, current_time += 3000, 103, 30);   // Payment verification
        pm7t_add_event(event_log, order_id, 3, current_time += 4000, 104, 60);   // Inventory check
        pm7t_add_event(event_log, order_id, 18, current_time += 5000, 105, 90);  // Order cancellation
        pm7t_add_event(event_log, order_id, 15, current_time += 6000, 106, 120); // Refund processing
        break;

      case 4:                                                                     // Inventory shortage scenario (40 instances)
        pm7t_add_event(event_log, order_id, 0, current_time += 1000, 101, 30);    // Order received
        pm7t_add_event(event_log, order_id, 1, current_time += 2000, 102, 45);    // Payment processing
        pm7t_add_event(event_log, order_id, 2, current_time += 3000, 103, 30);    // Payment verification
        pm7t_add_event(event_log, order_id, 3, current_time += 4000, 104, 60);    // Inventory check
        pm7t_add_event(event_log, order_id, 20, current_time += 5000, 105, 180);  // Supplier order
        pm7t_add_event(event_log, order_id, 21, current_time += 6000, 106, 3600); // Supplier delivery
        pm7t_add_event(event_log, order_id, 22, current_time += 7000, 107, 120);  // Warehouse receiving
        pm7t_add_event(event_log, order_id, 4, current_time += 8000, 108, 30);    // Order confirmation
        pm7t_add_event(event_log, order_id, 5, current_time += 9000, 109, 120);   // Pick and pack
        pm7t_add_event(event_log, order_id, 6, current_time += 10000, 110, 60);   // Quality check
        pm7t_add_event(event_log, order_id, 7, current_time += 11000, 111, 45);   // Shipping label
        pm7t_add_event(event_log, order_id, 8, current_time += 12000, 112, 30);   // Handoff to carrier
        pm7t_add_event(event_log, order_id, 9, current_time += 13000, 113, 1800); // In transit
        pm7t_add_event(event_log, order_id, 10, current_time += 14000, 114, 300); // Out for delivery
        pm7t_add_event(event_log, order_id, 11, current_time += 15000, 115, 60);  // Delivered
        break;

      case 5:                                                                    // Express shipping (40 instances)
        pm7t_add_event(event_log, order_id, 0, current_time += 500, 101, 20);    // Order received
        pm7t_add_event(event_log, order_id, 1, current_time += 1000, 102, 30);   // Payment processing
        pm7t_add_event(event_log, order_id, 2, current_time += 1500, 103, 20);   // Payment verification
        pm7t_add_event(event_log, order_id, 3, current_time += 2000, 104, 30);   // Inventory check
        pm7t_add_event(event_log, order_id, 4, current_time += 2500, 105, 20);   // Order confirmation
        pm7t_add_event(event_log, order_id, 5, current_time += 3000, 106, 60);   // Pick and pack
        pm7t_add_event(event_log, order_id, 6, current_time += 3500, 107, 30);   // Quality check
        pm7t_add_event(event_log, order_id, 7, current_time += 4000, 108, 30);   // Shipping label
        pm7t_add_event(event_log, order_id, 8, current_time += 4500, 109, 20);   // Handoff to carrier
        pm7t_add_event(event_log, order_id, 9, current_time += 5000, 110, 600);  // In transit (express)
        pm7t_add_event(event_log, order_id, 10, current_time += 5500, 111, 120); // Out for delivery
        pm7t_add_event(event_log, order_id, 11, current_time += 6000, 112, 30);  // Delivered
        break;

      case 6:                                                                     // Product catalog update (40 instances)
        pm7t_add_event(event_log, order_id, 0, current_time += 1000, 101, 30);    // Order received
        pm7t_add_event(event_log, order_id, 1, current_time += 2000, 102, 45);    // Payment processing
        pm7t_add_event(event_log, order_id, 2, current_time += 3000, 103, 30);    // Payment verification
        pm7t_add_event(event_log, order_id, 23, current_time += 4000, 104, 180);  // Product catalog update
        pm7t_add_event(event_log, order_id, 3, current_time += 5000, 105, 60);    // Inventory check
        pm7t_add_event(event_log, order_id, 4, current_time += 6000, 106, 30);    // Order confirmation
        pm7t_add_event(event_log, order_id, 5, current_time += 7000, 107, 120);   // Pick and pack
        pm7t_add_event(event_log, order_id, 6, current_time += 8000, 108, 60);    // Quality check
        pm7t_add_event(event_log, order_id, 7, current_time += 9000, 109, 45);    // Shipping label
        pm7t_add_event(event_log, order_id, 8, current_time += 10000, 110, 30);   // Handoff to carrier
        pm7t_add_event(event_log, order_id, 9, current_time += 11000, 111, 1800); // In transit
        pm7t_add_event(event_log, order_id, 10, current_time += 12000, 112, 300); // Out for delivery
        pm7t_add_event(event_log, order_id, 11, current_time += 13000, 113, 60);  // Delivered
        break;

      case 7:                                                                     // Multiple support interactions (40 instances)
        pm7t_add_event(event_log, order_id, 0, current_time += 1000, 101, 30);    // Order received
        pm7t_add_event(event_log, order_id, 1, current_time += 2000, 102, 45);    // Payment processing
        pm7t_add_event(event_log, order_id, 2, current_time += 3000, 103, 30);    // Payment verification
        pm7t_add_event(event_log, order_id, 3, current_time += 4000, 104, 60);    // Inventory check
        pm7t_add_event(event_log, order_id, 4, current_time += 5000, 105, 30);    // Order confirmation
        pm7t_add_event(event_log, order_id, 16, current_time += 6000, 106, 120);  // Support ticket 1
        pm7t_add_event(event_log, order_id, 17, current_time += 7000, 107, 180);  // Issue resolution 1
        pm7t_add_event(event_log, order_id, 16, current_time += 8000, 108, 120);  // Support ticket 2
        pm7t_add_event(event_log, order_id, 17, current_time += 9000, 109, 240);  // Issue resolution 2
        pm7t_add_event(event_log, order_id, 5, current_time += 10000, 110, 120);  // Pick and pack
        pm7t_add_event(event_log, order_id, 6, current_time += 11000, 111, 60);   // Quality check
        pm7t_add_event(event_log, order_id, 7, current_time += 12000, 112, 45);   // Shipping label
        pm7t_add_event(event_log, order_id, 8, current_time += 13000, 113, 30);   // Handoff to carrier
        pm7t_add_event(event_log, order_id, 9, current_time += 14000, 114, 1800); // In transit
        pm7t_add_event(event_log, order_id, 10, current_time += 15000, 115, 300); // Out for delivery
        pm7t_add_event(event_log, order_id, 11, current_time += 16000, 116, 60);  // Delivered
        break;

      case 8:                                                                     // International shipping (40 instances)
        pm7t_add_event(event_log, order_id, 0, current_time += 1000, 101, 30);    // Order received
        pm7t_add_event(event_log, order_id, 1, current_time += 2000, 102, 45);    // Payment processing
        pm7t_add_event(event_log, order_id, 2, current_time += 3000, 103, 30);    // Payment verification
        pm7t_add_event(event_log, order_id, 3, current_time += 4000, 104, 60);    // Inventory check
        pm7t_add_event(event_log, order_id, 4, current_time += 5000, 105, 30);    // Order confirmation
        pm7t_add_event(event_log, order_id, 5, current_time += 6000, 106, 120);   // Pick and pack
        pm7t_add_event(event_log, order_id, 6, current_time += 7000, 107, 60);    // Quality check
        pm7t_add_event(event_log, order_id, 7, current_time += 8000, 108, 45);    // Shipping label
        pm7t_add_event(event_log, order_id, 8, current_time += 9000, 109, 30);    // Handoff to carrier
        pm7t_add_event(event_log, order_id, 9, current_time += 10000, 110, 7200); // In transit (international)
        pm7t_add_event(event_log, order_id, 10, current_time += 11000, 111, 600); // Out for delivery
        pm7t_add_event(event_log, order_id, 11, current_time += 12000, 112, 120); // Delivered
        break;

      case 9:                                                                     // Bulk order processing (40 instances)
        pm7t_add_event(event_log, order_id, 0, current_time += 1000, 101, 30);    // Order received
        pm7t_add_event(event_log, order_id, 1, current_time += 2000, 102, 45);    // Payment processing
        pm7t_add_event(event_log, order_id, 2, current_time += 3000, 103, 30);    // Payment verification
        pm7t_add_event(event_log, order_id, 3, current_time += 4000, 104, 60);    // Inventory check
        pm7t_add_event(event_log, order_id, 4, current_time += 5000, 105, 30);    // Order confirmation
        pm7t_add_event(event_log, order_id, 5, current_time += 6000, 106, 300);   // Pick and pack (bulk)
        pm7t_add_event(event_log, order_id, 6, current_time += 7000, 107, 120);   // Quality check
        pm7t_add_event(event_log, order_id, 7, current_time += 8000, 108, 45);    // Shipping label
        pm7t_add_event(event_log, order_id, 8, current_time += 9000, 109, 30);    // Handoff to carrier
        pm7t_add_event(event_log, order_id, 9, current_time += 10000, 110, 2400); // In transit
        pm7t_add_event(event_log, order_id, 10, current_time += 11000, 111, 300); // Out for delivery
        pm7t_add_event(event_log, order_id, 11, current_time += 12000, 112, 60);  // Delivered
        break;
      }

      order_id++;
    }
  }

  printf("Generated %zu events across %u order processes\n",
         pm7t_get_event_count(event_log), order_id - 1);
  printf("Unique activities: %u\n", pm7t_get_unique_activities(event_log));
  printf("Unique resources (staff): %u\n", pm7t_get_unique_resources(event_log));

  // Extract order fulfillment traces
  printf("\nExtracting order fulfillment traces...\n");
  TraceLog *trace_log = pm7t_extract_traces(event_log);
  if (!trace_log)
  {
    fprintf(stderr, "Failed to extract order fulfillment traces\n");
    pm7t_destroy_event_log(event_log);
    return 1;
  }

  printf("Extracted %zu order fulfillment traces\n", pm7t_get_trace_count(trace_log));

  // Display sample order processes
  printf("\nSample order fulfillment processes:\n");
  for (size_t i = 0; i < 5 && i < pm7t_get_trace_count(trace_log); i++)
  {
    Trace *trace = pm7t_get_trace(trace_log, i);
    printf("Order Process %zu: ", i + 1);
    for (size_t j = 0; j < trace->size; j++)
    {
      printf("%s", activities[trace->activities[j]]);
      if (j < trace->size - 1)
        printf(" -> ");
    }
    printf("\n");
  }

  // Process discovery for e-commerce workflows
  printf("\n=== E-commerce Process Discovery ===\n");

  // Alpha algorithm for e-commerce processes
  printf("Discovering e-commerce process model using Alpha algorithm...\n");
  ProcessModel *alpha_model = pm7t_discover_alpha_algorithm(trace_log);
  if (alpha_model)
  {
    printf("Alpha algorithm discovered %zu e-commerce process transitions:\n", alpha_model->size);
    for (size_t i = 0; i < alpha_model->size; i++)
    {
      printf("  %s -> %s (freq: %u, prob: %.3f)\n",
             activities[alpha_model->transitions[i].from_activity],
             activities[alpha_model->transitions[i].to_activity],
             alpha_model->transitions[i].frequency,
             alpha_model->transitions[i].probability);
    }
  }

  // Heuristic miner for e-commerce processes
  printf("\nDiscovering e-commerce process model using Heuristic miner...\n");
  ProcessModel *heuristic_model = pm7t_discover_heuristic_miner(trace_log, 0.3);
  if (heuristic_model)
  {
    printf("Heuristic miner discovered %zu e-commerce process transitions:\n", heuristic_model->size);
    for (size_t i = 0; i < heuristic_model->size; i++)
    {
      printf("  %s -> %s (freq: %u, prob: %.3f)\n",
             activities[heuristic_model->transitions[i].from_activity],
             activities[heuristic_model->transitions[i].to_activity],
             heuristic_model->transitions[i].frequency,
             heuristic_model->transitions[i].probability);
    }
  }

  // E-commerce process analysis
  printf("\n=== E-commerce Process Analysis ===\n");
  ProcessStats *process_stats = pm7t_analyze_process(event_log);
  if (process_stats)
  {
    printf("E-commerce activity statistics:\n");
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

  // Conformance checking for e-commerce processes
  printf("\n=== E-commerce Process Conformance ===\n");
  if (alpha_model)
  {
    ConformanceResult alpha_conformance = pm7t_check_conformance(alpha_model, trace_log);
    printf("Alpha algorithm conformance for e-commerce processes:\n");
    printf("  Fitness: %.3f (how well the model fits order processes)\n", alpha_conformance.fitness);
    printf("  Precision: %.3f (how precise the e-commerce model is)\n", alpha_conformance.precision);
    printf("  Generalization: %.3f (how well the model generalizes)\n", alpha_conformance.generalization);
    printf("  Simplicity: %.3f (how simple the e-commerce model is)\n", alpha_conformance.simplicity);
  }

  // Performance analysis for e-commerce processes
  printf("\n=== E-commerce Performance Analysis ===\n");
  PerformanceAnalysis *performance = pm7t_analyze_performance(event_log);
  if (performance)
  {
    printf("E-commerce performance metrics:\n");
    printf("  Total orders processed: %zu\n", performance->size);
    printf("  Average order fulfillment time: %.2f hours\n",
           performance->avg_duration / 3600000000000.0); // Convert to hours
    printf("  Minimum order time: %.2f minutes\n",
           performance->min_duration / 60000000000.0); // Convert to minutes
    printf("  Maximum order time: %.2f hours\n",
           performance->max_duration / 3600000000000.0); // Convert to hours
    printf("  Order throughput: %.2f orders/day\n",
           performance->throughput * 86400); // Convert to orders per day

    // Identify slow orders
    printf("\nSlow orders (fulfillment >24 hours):\n");
    int slow_order_count = 0;
    for (size_t i = 0; i < performance->size; i++)
    {
      double duration_hours = (double)performance->cases[i].duration / 3600000000000.0;
      if (duration_hours > 24.0)
      {
        printf("  Order %u: %.2f hours, %u activities\n",
               performance->cases[i].case_id,
               duration_hours,
               performance->cases[i].num_activities);
        slow_order_count++;
      }
    }
    printf("  Total slow orders: %d (%.1f%%)\n",
           slow_order_count, (double)slow_order_count / performance->size * 100);
  }

  // Bottleneck analysis for e-commerce processes
  printf("\n=== E-commerce Bottleneck Analysis ===\n");
  BottleneckAnalysis *bottlenecks = pm7t_analyze_bottlenecks(event_log);
  if (bottlenecks)
  {
    printf("E-commerce process bottlenecks:\n");
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

  // Variant analysis for e-commerce processes
  printf("\n=== E-commerce Order Variants ===\n");
  VariantAnalysis *variants = pm7t_analyze_variants(trace_log);
  if (variants)
  {
    printf("Order fulfillment variants (top 10):\n");
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

  // Social network analysis for e-commerce staff
  printf("\n=== E-commerce Staff Collaboration Network ===\n");
  SocialNetwork *network = pm7t_analyze_social_network(event_log);
  if (network)
  {
    printf("E-commerce staff collaboration patterns:\n");
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

  printf("\nE-commerce process mining analysis completed successfully!\n");
  printf("Memory usage: %zu bytes\n", pm7t_get_memory_usage());

  return 0;
}