#include "../c_src/pm7t.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// Manufacturing process mining demo
int main()
{
  printf("=== 7T Manufacturing Process Mining Demo ===\n");
  printf("Analyzing production line and quality control workflows\n\n");

  // Set memory limit (2GB for manufacturing data)
  pm7t_set_memory_limit(2ULL * 1024 * 1024 * 1024);

  // Create event log for manufacturing processes
  EventLog *event_log = pm7t_create_event_log(60000);
  if (!event_log)
  {
    fprintf(stderr, "Failed to create manufacturing event log\n");
    return 1;
  }

  // Manufacturing activity definitions
  const char *activities[] = {
      "Raw Material Receiving",
      "Quality Inspection (Incoming)",
      "Material Storage",
      "Production Planning",
      "Work Order Creation",
      "Machine Setup",
      "Assembly Line Start",
      "Component Assembly",
      "Sub-assembly Production",
      "Main Assembly",
      "Quality Check (In-Process)",
      "Testing and Calibration",
      "Final Quality Inspection",
      "Packaging",
      "Labeling",
      "Warehouse Storage",
      "Order Picking",
      "Shipping Preparation",
      "Outbound Logistics",
      "Customer Delivery",
      "Defect Detection",
      "Rework Process",
      "Scrap Disposal",
      "Maintenance Schedule",
      "Preventive Maintenance",
      "Emergency Repair",
      "Supplier Order",
      "Supplier Delivery",
      "Inventory Count",
      "Material Transfer",
      "Production Line Changeover",
      "Safety Inspection",
      "Environmental Compliance Check"};

  printf("Generating synthetic manufacturing process data...\n");

  // Generate synthetic manufacturing process data
  uint64_t base_time = time(NULL) * 1000000000ULL;
  uint32_t product_id = 1;
  srand(42); // For reproducible results

  // Generate different manufacturing scenarios
  for (int scenario = 0; scenario < 12; scenario++)
  {
    int instances = (scenario == 0) ? 100 : 50; // More instances for standard production

    for (int instance = 0; instance < instances; instance++)
    {
      uint64_t production_start = base_time + (product_id * 1000000);
      uint64_t current_time = production_start;

      switch (scenario)
      {
      case 0:                                                                       // Standard production line (100 instances)
        pm7t_add_event(event_log, product_id, 0, current_time += 1000, 101, 60);    // Raw material receiving
        pm7t_add_event(event_log, product_id, 1, current_time += 2000, 102, 90);    // Quality inspection
        pm7t_add_event(event_log, product_id, 2, current_time += 3000, 103, 30);    // Material storage
        pm7t_add_event(event_log, product_id, 3, current_time += 4000, 104, 120);   // Production planning
        pm7t_add_event(event_log, product_id, 4, current_time += 5000, 105, 60);    // Work order creation
        pm7t_add_event(event_log, product_id, 5, current_time += 6000, 106, 180);   // Machine setup
        pm7t_add_event(event_log, product_id, 6, current_time += 7000, 107, 60);    // Assembly line start
        pm7t_add_event(event_log, product_id, 7, current_time += 8000, 108, 240);   // Component assembly
        pm7t_add_event(event_log, product_id, 8, current_time += 9000, 109, 300);   // Sub-assembly
        pm7t_add_event(event_log, product_id, 9, current_time += 10000, 110, 360);  // Main assembly
        pm7t_add_event(event_log, product_id, 10, current_time += 11000, 111, 120); // Quality check
        pm7t_add_event(event_log, product_id, 11, current_time += 12000, 112, 180); // Testing
        pm7t_add_event(event_log, product_id, 12, current_time += 13000, 113, 90);  // Final inspection
        pm7t_add_event(event_log, product_id, 13, current_time += 14000, 114, 60);  // Packaging
        pm7t_add_event(event_log, product_id, 14, current_time += 15000, 115, 30);  // Labeling
        pm7t_add_event(event_log, product_id, 15, current_time += 16000, 116, 45);  // Warehouse storage
        break;

      case 1:                                                                       // Production with quality issues (50 instances)
        pm7t_add_event(event_log, product_id, 0, current_time += 1000, 101, 60);    // Raw material receiving
        pm7t_add_event(event_log, product_id, 1, current_time += 2000, 102, 90);    // Quality inspection
        pm7t_add_event(event_log, product_id, 2, current_time += 3000, 103, 30);    // Material storage
        pm7t_add_event(event_log, product_id, 3, current_time += 4000, 104, 120);   // Production planning
        pm7t_add_event(event_log, product_id, 4, current_time += 5000, 105, 60);    // Work order creation
        pm7t_add_event(event_log, product_id, 5, current_time += 6000, 106, 180);   // Machine setup
        pm7t_add_event(event_log, product_id, 6, current_time += 7000, 107, 60);    // Assembly line start
        pm7t_add_event(event_log, product_id, 7, current_time += 8000, 108, 240);   // Component assembly
        pm7t_add_event(event_log, product_id, 10, current_time += 9000, 109, 120);  // Quality check
        pm7t_add_event(event_log, product_id, 20, current_time += 10000, 110, 60);  // Defect detection
        pm7t_add_event(event_log, product_id, 21, current_time += 11000, 111, 300); // Rework process
        pm7t_add_event(event_log, product_id, 8, current_time += 12000, 112, 300);  // Sub-assembly
        pm7t_add_event(event_log, product_id, 9, current_time += 13000, 113, 360);  // Main assembly
        pm7t_add_event(event_log, product_id, 10, current_time += 14000, 114, 120); // Quality check
        pm7t_add_event(event_log, product_id, 11, current_time += 15000, 115, 180); // Testing
        pm7t_add_event(event_log, product_id, 12, current_time += 16000, 116, 90);  // Final inspection
        pm7t_add_event(event_log, product_id, 13, current_time += 17000, 117, 60);  // Packaging
        pm7t_add_event(event_log, product_id, 14, current_time += 18000, 118, 30);  // Labeling
        pm7t_add_event(event_log, product_id, 15, current_time += 19000, 119, 45);  // Warehouse storage
        break;

      case 2:                                                                       // Production with maintenance (50 instances)
        pm7t_add_event(event_log, product_id, 0, current_time += 1000, 101, 60);    // Raw material receiving
        pm7t_add_event(event_log, product_id, 1, current_time += 2000, 102, 90);    // Quality inspection
        pm7t_add_event(event_log, product_id, 2, current_time += 3000, 103, 30);    // Material storage
        pm7t_add_event(event_log, product_id, 3, current_time += 4000, 104, 120);   // Production planning
        pm7t_add_event(event_log, product_id, 4, current_time += 5000, 105, 60);    // Work order creation
        pm7t_add_event(event_log, product_id, 24, current_time += 6000, 106, 240);  // Maintenance schedule
        pm7t_add_event(event_log, product_id, 25, current_time += 7000, 107, 360);  // Preventive maintenance
        pm7t_add_event(event_log, product_id, 5, current_time += 8000, 108, 180);   // Machine setup
        pm7t_add_event(event_log, product_id, 6, current_time += 9000, 109, 60);    // Assembly line start
        pm7t_add_event(event_log, product_id, 7, current_time += 10000, 110, 240);  // Component assembly
        pm7t_add_event(event_log, product_id, 8, current_time += 11000, 111, 300);  // Sub-assembly
        pm7t_add_event(event_log, product_id, 9, current_time += 12000, 112, 360);  // Main assembly
        pm7t_add_event(event_log, product_id, 10, current_time += 13000, 113, 120); // Quality check
        pm7t_add_event(event_log, product_id, 11, current_time += 14000, 114, 180); // Testing
        pm7t_add_event(event_log, product_id, 12, current_time += 15000, 115, 90);  // Final inspection
        pm7t_add_event(event_log, product_id, 13, current_time += 16000, 116, 60);  // Packaging
        pm7t_add_event(event_log, product_id, 14, current_time += 17000, 117, 30);  // Labeling
        pm7t_add_event(event_log, product_id, 15, current_time += 18000, 118, 45);  // Warehouse storage
        break;

      case 3:                                                                       // Production with scrap (50 instances)
        pm7t_add_event(event_log, product_id, 0, current_time += 1000, 101, 60);    // Raw material receiving
        pm7t_add_event(event_log, product_id, 1, current_time += 2000, 102, 90);    // Quality inspection
        pm7t_add_event(event_log, product_id, 2, current_time += 3000, 103, 30);    // Material storage
        pm7t_add_event(event_log, product_id, 3, current_time += 4000, 104, 120);   // Production planning
        pm7t_add_event(event_log, product_id, 4, current_time += 5000, 105, 60);    // Work order creation
        pm7t_add_event(event_log, product_id, 5, current_time += 6000, 106, 180);   // Machine setup
        pm7t_add_event(event_log, product_id, 6, current_time += 7000, 107, 60);    // Assembly line start
        pm7t_add_event(event_log, product_id, 7, current_time += 8000, 108, 240);   // Component assembly
        pm7t_add_event(event_log, product_id, 10, current_time += 9000, 109, 120);  // Quality check
        pm7t_add_event(event_log, product_id, 20, current_time += 10000, 110, 60);  // Defect detection
        pm7t_add_event(event_log, product_id, 22, current_time += 11000, 111, 120); // Scrap disposal
        break;

      case 4:                                                                       // Emergency maintenance scenario (50 instances)
        pm7t_add_event(event_log, product_id, 0, current_time += 1000, 101, 60);    // Raw material receiving
        pm7t_add_event(event_log, product_id, 1, current_time += 2000, 102, 90);    // Quality inspection
        pm7t_add_event(event_log, product_id, 2, current_time += 3000, 103, 30);    // Material storage
        pm7t_add_event(event_log, product_id, 3, current_time += 4000, 104, 120);   // Production planning
        pm7t_add_event(event_log, product_id, 4, current_time += 5000, 105, 60);    // Work order creation
        pm7t_add_event(event_log, product_id, 5, current_time += 6000, 106, 180);   // Machine setup
        pm7t_add_event(event_log, product_id, 6, current_time += 7000, 107, 60);    // Assembly line start
        pm7t_add_event(event_log, product_id, 26, current_time += 8000, 108, 480);  // Emergency repair
        pm7t_add_event(event_log, product_id, 7, current_time += 9000, 109, 240);   // Component assembly
        pm7t_add_event(event_log, product_id, 8, current_time += 10000, 110, 300);  // Sub-assembly
        pm7t_add_event(event_log, product_id, 9, current_time += 11000, 111, 360);  // Main assembly
        pm7t_add_event(event_log, product_id, 10, current_time += 12000, 112, 120); // Quality check
        pm7t_add_event(event_log, product_id, 11, current_time += 13000, 113, 180); // Testing
        pm7t_add_event(event_log, product_id, 12, current_time += 14000, 114, 90);  // Final inspection
        pm7t_add_event(event_log, product_id, 13, current_time += 15000, 115, 60);  // Packaging
        pm7t_add_event(event_log, product_id, 14, current_time += 16000, 116, 30);  // Labeling
        pm7t_add_event(event_log, product_id, 15, current_time += 17000, 117, 45);  // Warehouse storage
        break;

      case 5:                                                                       // Supply chain disruption (50 instances)
        pm7t_add_event(event_log, product_id, 0, current_time += 1000, 101, 60);    // Raw material receiving
        pm7t_add_event(event_log, product_id, 1, current_time += 2000, 102, 90);    // Quality inspection
        pm7t_add_event(event_log, product_id, 2, current_time += 3000, 103, 30);    // Material storage
        pm7t_add_event(event_log, product_id, 26, current_time += 4000, 104, 240);  // Supplier order
        pm7t_add_event(event_log, product_id, 27, current_time += 5000, 105, 3600); // Supplier delivery
        pm7t_add_event(event_log, product_id, 3, current_time += 6000, 106, 120);   // Production planning
        pm7t_add_event(event_log, product_id, 4, current_time += 7000, 107, 60);    // Work order creation
        pm7t_add_event(event_log, product_id, 5, current_time += 8000, 108, 180);   // Machine setup
        pm7t_add_event(event_log, product_id, 6, current_time += 9000, 109, 60);    // Assembly line start
        pm7t_add_event(event_log, product_id, 7, current_time += 10000, 110, 240);  // Component assembly
        pm7t_add_event(event_log, product_id, 8, current_time += 11000, 111, 300);  // Sub-assembly
        pm7t_add_event(event_log, product_id, 9, current_time += 12000, 112, 360);  // Main assembly
        pm7t_add_event(event_log, product_id, 10, current_time += 13000, 113, 120); // Quality check
        pm7t_add_event(event_log, product_id, 11, current_time += 14000, 114, 180); // Testing
        pm7t_add_event(event_log, product_id, 12, current_time += 15000, 115, 90);  // Final inspection
        pm7t_add_event(event_log, product_id, 13, current_time += 16000, 116, 60);  // Packaging
        pm7t_add_event(event_log, product_id, 14, current_time += 17000, 117, 30);  // Labeling
        pm7t_add_event(event_log, product_id, 15, current_time += 18000, 118, 45);  // Warehouse storage
        break;

      case 6:                                                                       // Production line changeover (50 instances)
        pm7t_add_event(event_log, product_id, 0, current_time += 1000, 101, 60);    // Raw material receiving
        pm7t_add_event(event_log, product_id, 1, current_time += 2000, 102, 90);    // Quality inspection
        pm7t_add_event(event_log, product_id, 2, current_time += 3000, 103, 30);    // Material storage
        pm7t_add_event(event_log, product_id, 3, current_time += 4000, 104, 120);   // Production planning
        pm7t_add_event(event_log, product_id, 4, current_time += 5000, 105, 60);    // Work order creation
        pm7t_add_event(event_log, product_id, 30, current_time += 6000, 106, 300);  // Production line changeover
        pm7t_add_event(event_log, product_id, 5, current_time += 7000, 107, 180);   // Machine setup
        pm7t_add_event(event_log, product_id, 6, current_time += 8000, 108, 60);    // Assembly line start
        pm7t_add_event(event_log, product_id, 7, current_time += 9000, 109, 240);   // Component assembly
        pm7t_add_event(event_log, product_id, 8, current_time += 10000, 110, 300);  // Sub-assembly
        pm7t_add_event(event_log, product_id, 9, current_time += 11000, 111, 360);  // Main assembly
        pm7t_add_event(event_log, product_id, 10, current_time += 12000, 112, 120); // Quality check
        pm7t_add_event(event_log, product_id, 11, current_time += 13000, 113, 180); // Testing
        pm7t_add_event(event_log, product_id, 12, current_time += 14000, 114, 90);  // Final inspection
        pm7t_add_event(event_log, product_id, 13, current_time += 15000, 115, 60);  // Packaging
        pm7t_add_event(event_log, product_id, 14, current_time += 16000, 116, 30);  // Labeling
        pm7t_add_event(event_log, product_id, 15, current_time += 17000, 117, 45);  // Warehouse storage
        break;

      case 7:                                                                       // Safety and compliance focused (50 instances)
        pm7t_add_event(event_log, product_id, 0, current_time += 1000, 101, 60);    // Raw material receiving
        pm7t_add_event(event_log, product_id, 1, current_time += 2000, 102, 90);    // Quality inspection
        pm7t_add_event(event_log, product_id, 2, current_time += 3000, 103, 30);    // Material storage
        pm7t_add_event(event_log, product_id, 31, current_time += 4000, 104, 120);  // Safety inspection
        pm7t_add_event(event_log, product_id, 3, current_time += 5000, 105, 120);   // Production planning
        pm7t_add_event(event_log, product_id, 4, current_time += 6000, 106, 60);    // Work order creation
        pm7t_add_event(event_log, product_id, 5, current_time += 7000, 107, 180);   // Machine setup
        pm7t_add_event(event_log, product_id, 6, current_time += 8000, 108, 60);    // Assembly line start
        pm7t_add_event(event_log, product_id, 7, current_time += 9000, 109, 240);   // Component assembly
        pm7t_add_event(event_log, product_id, 8, current_time += 10000, 110, 300);  // Sub-assembly
        pm7t_add_event(event_log, product_id, 9, current_time += 11000, 111, 360);  // Main assembly
        pm7t_add_event(event_log, product_id, 10, current_time += 12000, 112, 120); // Quality check
        pm7t_add_event(event_log, product_id, 11, current_time += 13000, 113, 180); // Testing
        pm7t_add_event(event_log, product_id, 32, current_time += 14000, 114, 90);  // Environmental compliance
        pm7t_add_event(event_log, product_id, 12, current_time += 15000, 115, 90);  // Final inspection
        pm7t_add_event(event_log, product_id, 13, current_time += 16000, 116, 60);  // Packaging
        pm7t_add_event(event_log, product_id, 14, current_time += 17000, 117, 30);  // Labeling
        pm7t_add_event(event_log, product_id, 15, current_time += 18000, 118, 45);  // Warehouse storage
        break;

      case 8:                                                                       // Inventory management intensive (50 instances)
        pm7t_add_event(event_log, product_id, 0, current_time += 1000, 101, 60);    // Raw material receiving
        pm7t_add_event(event_log, product_id, 1, current_time += 2000, 102, 90);    // Quality inspection
        pm7t_add_event(event_log, product_id, 2, current_time += 3000, 103, 30);    // Material storage
        pm7t_add_event(event_log, product_id, 28, current_time += 4000, 104, 180);  // Inventory count
        pm7t_add_event(event_log, product_id, 29, current_time += 5000, 105, 120);  // Material transfer
        pm7t_add_event(event_log, product_id, 3, current_time += 6000, 106, 120);   // Production planning
        pm7t_add_event(event_log, product_id, 4, current_time += 7000, 107, 60);    // Work order creation
        pm7t_add_event(event_log, product_id, 5, current_time += 8000, 108, 180);   // Machine setup
        pm7t_add_event(event_log, product_id, 6, current_time += 9000, 109, 60);    // Assembly line start
        pm7t_add_event(event_log, product_id, 7, current_time += 10000, 110, 240);  // Component assembly
        pm7t_add_event(event_log, product_id, 8, current_time += 11000, 111, 300);  // Sub-assembly
        pm7t_add_event(event_log, product_id, 9, current_time += 12000, 112, 360);  // Main assembly
        pm7t_add_event(event_log, product_id, 10, current_time += 13000, 113, 120); // Quality check
        pm7t_add_event(event_log, product_id, 11, current_time += 14000, 114, 180); // Testing
        pm7t_add_event(event_log, product_id, 12, current_time += 15000, 115, 90);  // Final inspection
        pm7t_add_event(event_log, product_id, 13, current_time += 16000, 116, 60);  // Packaging
        pm7t_add_event(event_log, product_id, 14, current_time += 17000, 117, 30);  // Labeling
        pm7t_add_event(event_log, product_id, 15, current_time += 18000, 118, 45);  // Warehouse storage
        break;

      case 9:                                                                     // High-volume production (50 instances)
        pm7t_add_event(event_log, product_id, 0, current_time += 500, 101, 30);   // Raw material receiving
        pm7t_add_event(event_log, product_id, 1, current_time += 1000, 102, 45);  // Quality inspection
        pm7t_add_event(event_log, product_id, 2, current_time += 1500, 103, 15);  // Material storage
        pm7t_add_event(event_log, product_id, 3, current_time += 2000, 104, 60);  // Production planning
        pm7t_add_event(event_log, product_id, 4, current_time += 2500, 105, 30);  // Work order creation
        pm7t_add_event(event_log, product_id, 5, current_time += 3000, 106, 90);  // Machine setup
        pm7t_add_event(event_log, product_id, 6, current_time += 3500, 107, 30);  // Assembly line start
        pm7t_add_event(event_log, product_id, 7, current_time += 4000, 108, 120); // Component assembly
        pm7t_add_event(event_log, product_id, 8, current_time += 4500, 109, 150); // Sub-assembly
        pm7t_add_event(event_log, product_id, 9, current_time += 5000, 110, 180); // Main assembly
        pm7t_add_event(event_log, product_id, 10, current_time += 5500, 111, 60); // Quality check
        pm7t_add_event(event_log, product_id, 11, current_time += 6000, 112, 90); // Testing
        pm7t_add_event(event_log, product_id, 12, current_time += 6500, 113, 45); // Final inspection
        pm7t_add_event(event_log, product_id, 13, current_time += 7000, 114, 30); // Packaging
        pm7t_add_event(event_log, product_id, 14, current_time += 7500, 115, 15); // Labeling
        pm7t_add_event(event_log, product_id, 15, current_time += 8000, 116, 22); // Warehouse storage
        break;

      case 10:                                                                      // Custom manufacturing (50 instances)
        pm7t_add_event(event_log, product_id, 0, current_time += 1000, 101, 60);    // Raw material receiving
        pm7t_add_event(event_log, product_id, 1, current_time += 2000, 102, 90);    // Quality inspection
        pm7t_add_event(event_log, product_id, 2, current_time += 3000, 103, 30);    // Material storage
        pm7t_add_event(event_log, product_id, 3, current_time += 4000, 104, 240);   // Production planning
        pm7t_add_event(event_log, product_id, 4, current_time += 5000, 105, 120);   // Work order creation
        pm7t_add_event(event_log, product_id, 5, current_time += 6000, 106, 360);   // Machine setup
        pm7t_add_event(event_log, product_id, 6, current_time += 7000, 107, 60);    // Assembly line start
        pm7t_add_event(event_log, product_id, 7, current_time += 8000, 108, 480);   // Component assembly
        pm7t_add_event(event_log, product_id, 8, current_time += 9000, 109, 600);   // Sub-assembly
        pm7t_add_event(event_log, product_id, 9, current_time += 10000, 110, 720);  // Main assembly
        pm7t_add_event(event_log, product_id, 10, current_time += 11000, 111, 240); // Quality check
        pm7t_add_event(event_log, product_id, 11, current_time += 12000, 112, 360); // Testing
        pm7t_add_event(event_log, product_id, 12, current_time += 13000, 113, 180); // Final inspection
        pm7t_add_event(event_log, product_id, 13, current_time += 14000, 114, 120); // Packaging
        pm7t_add_event(event_log, product_id, 14, current_time += 15000, 115, 60);  // Labeling
        pm7t_add_event(event_log, product_id, 15, current_time += 16000, 116, 90);  // Warehouse storage
        break;

      case 11:                                                                    // Just-in-time production (50 instances)
        pm7t_add_event(event_log, product_id, 0, current_time += 300, 101, 20);   // Raw material receiving
        pm7t_add_event(event_log, product_id, 1, current_time += 600, 102, 30);   // Quality inspection
        pm7t_add_event(event_log, product_id, 2, current_time += 900, 103, 10);   // Material storage
        pm7t_add_event(event_log, product_id, 3, current_time += 1200, 104, 30);  // Production planning
        pm7t_add_event(event_log, product_id, 4, current_time += 1500, 105, 15);  // Work order creation
        pm7t_add_event(event_log, product_id, 5, current_time += 1800, 106, 45);  // Machine setup
        pm7t_add_event(event_log, product_id, 6, current_time += 2100, 107, 15);  // Assembly line start
        pm7t_add_event(event_log, product_id, 7, current_time += 2400, 108, 60);  // Component assembly
        pm7t_add_event(event_log, product_id, 8, current_time += 2700, 109, 75);  // Sub-assembly
        pm7t_add_event(event_log, product_id, 9, current_time += 3000, 110, 90);  // Main assembly
        pm7t_add_event(event_log, product_id, 10, current_time += 3300, 111, 30); // Quality check
        pm7t_add_event(event_log, product_id, 11, current_time += 3600, 112, 45); // Testing
        pm7t_add_event(event_log, product_id, 12, current_time += 3900, 113, 22); // Final inspection
        pm7t_add_event(event_log, product_id, 13, current_time += 4200, 114, 15); // Packaging
        pm7t_add_event(event_log, product_id, 14, current_time += 4500, 115, 7);  // Labeling
        pm7t_add_event(event_log, product_id, 15, current_time += 4800, 116, 11); // Warehouse storage
        break;
      }

      product_id++;
    }
  }

  printf("Generated %zu events across %u manufacturing processes\n",
         pm7t_get_event_count(event_log), product_id - 1);
  printf("Unique activities: %u\n", pm7t_get_unique_activities(event_log));
  printf("Unique resources (machines/workers): %u\n", pm7t_get_unique_resources(event_log));

  // Extract manufacturing process traces
  printf("\nExtracting manufacturing process traces...\n");
  TraceLog *trace_log = pm7t_extract_traces(event_log);
  if (!trace_log)
  {
    fprintf(stderr, "Failed to extract manufacturing process traces\n");
    pm7t_destroy_event_log(event_log);
    return 1;
  }

  printf("Extracted %zu manufacturing process traces\n", pm7t_get_trace_count(trace_log));

  // Display sample manufacturing processes
  printf("\nSample manufacturing processes:\n");
  for (size_t i = 0; i < 5 && i < pm7t_get_trace_count(trace_log); i++)
  {
    Trace *trace = pm7t_get_trace(trace_log, i);
    printf("Manufacturing Process %zu: ", i + 1);
    for (size_t j = 0; j < trace->size; j++)
    {
      printf("%s", activities[trace->activities[j]]);
      if (j < trace->size - 1)
        printf(" -> ");
    }
    printf("\n");
  }

  // Process discovery for manufacturing workflows
  printf("\n=== Manufacturing Process Discovery ===\n");

  // Alpha algorithm for manufacturing processes
  printf("Discovering manufacturing process model using Alpha algorithm...\n");
  ProcessModel *alpha_model = pm7t_discover_alpha_algorithm(trace_log);
  if (alpha_model)
  {
    printf("Alpha algorithm discovered %zu manufacturing process transitions:\n", alpha_model->size);
    for (size_t i = 0; i < alpha_model->size; i++)
    {
      printf("  %s -> %s (freq: %u, prob: %.3f)\n",
             activities[alpha_model->transitions[i].from_activity],
             activities[alpha_model->transitions[i].to_activity],
             alpha_model->transitions[i].frequency,
             alpha_model->transitions[i].probability);
    }
  }

  // Heuristic miner for manufacturing processes
  printf("\nDiscovering manufacturing process model using Heuristic miner...\n");
  ProcessModel *heuristic_model = pm7t_discover_heuristic_miner(trace_log, 0.4);
  if (heuristic_model)
  {
    printf("Heuristic miner discovered %zu manufacturing process transitions:\n", heuristic_model->size);
    for (size_t i = 0; i < heuristic_model->size; i++)
    {
      printf("  %s -> %s (freq: %u, prob: %.3f)\n",
             activities[heuristic_model->transitions[i].from_activity],
             activities[heuristic_model->transitions[i].to_activity],
             heuristic_model->transitions[i].frequency,
             heuristic_model->transitions[i].probability);
    }
  }

  // Manufacturing process analysis
  printf("\n=== Manufacturing Process Analysis ===\n");
  ProcessStats *process_stats = pm7t_analyze_process(event_log);
  if (process_stats)
  {
    printf("Manufacturing activity statistics:\n");
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

  // Conformance checking for manufacturing processes
  printf("\n=== Manufacturing Process Conformance ===\n");
  if (alpha_model)
  {
    ConformanceResult alpha_conformance = pm7t_check_conformance(alpha_model, trace_log);
    printf("Alpha algorithm conformance for manufacturing processes:\n");
    printf("  Fitness: %.3f (how well the model fits production processes)\n", alpha_conformance.fitness);
    printf("  Precision: %.3f (how precise the manufacturing model is)\n", alpha_conformance.precision);
    printf("  Generalization: %.3f (how well the model generalizes)\n", alpha_conformance.generalization);
    printf("  Simplicity: %.3f (how simple the manufacturing model is)\n", alpha_conformance.simplicity);
  }

  // Performance analysis for manufacturing processes
  printf("\n=== Manufacturing Performance Analysis ===\n");
  PerformanceAnalysis *performance = pm7t_analyze_performance(event_log);
  if (performance)
  {
    printf("Manufacturing performance metrics:\n");
    printf("  Total products manufactured: %zu\n", performance->size);
    printf("  Average production time: %.2f hours\n",
           performance->avg_duration / 3600000000000.0); // Convert to hours
    printf("  Minimum production time: %.2f minutes\n",
           performance->min_duration / 60000000000.0); // Convert to minutes
    printf("  Maximum production time: %.2f hours\n",
           performance->max_duration / 3600000000000.0); // Convert to hours
    printf("  Production throughput: %.2f products/day\n",
           performance->throughput * 86400); // Convert to products per day

    // Identify production delays
    printf("\nProduction delays (manufacturing >8 hours):\n");
    int delay_count = 0;
    for (size_t i = 0; i < performance->size; i++)
    {
      double duration_hours = (double)performance->cases[i].duration / 3600000000000.0;
      if (duration_hours > 8.0)
      {
        printf("  Product %u: %.2f hours, %u activities\n",
               performance->cases[i].case_id,
               duration_hours,
               performance->cases[i].num_activities);
        delay_count++;
      }
    }
    printf("  Total delayed products: %d (%.1f%%)\n",
           delay_count, (double)delay_count / performance->size * 100);
  }

  // Bottleneck analysis for manufacturing processes
  printf("\n=== Manufacturing Bottleneck Analysis ===\n");
  BottleneckAnalysis *bottlenecks = pm7t_analyze_bottlenecks(event_log);
  if (bottlenecks)
  {
    printf("Manufacturing process bottlenecks:\n");
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

  // Variant analysis for manufacturing processes
  printf("\n=== Manufacturing Process Variants ===\n");
  VariantAnalysis *variants = pm7t_analyze_variants(trace_log);
  if (variants)
  {
    printf("Manufacturing process variants (top 10):\n");
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

  // Social network analysis for manufacturing resources
  printf("\n=== Manufacturing Resource Collaboration Network ===\n");
  SocialNetwork *network = pm7t_analyze_social_network(event_log);
  if (network)
  {
    printf("Manufacturing resource collaboration patterns:\n");
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

  printf("\nManufacturing process mining analysis completed successfully!\n");
  printf("Memory usage: %zu bytes\n", pm7t_get_memory_usage());

  return 0;
}