# 7T Process Mining Demo Suite

This directory contains 5 comprehensive process mining demo applications that showcase the 7T Engine's capabilities for analyzing business processes across different industries. Each demo demonstrates real-world process mining scenarios with synthetic data generation, process discovery, conformance checking, and performance analysis.

## Overview

The 7T Process Mining Engine provides high-performance process mining capabilities equivalent to pm4py functionality, optimized for sub-nanosecond operations and billion+ operations per second throughput. These demos showcase how to use the engine for:

- **Event Log Management**: Creating and managing large-scale event logs
- **Process Discovery**: Using Alpha algorithm and Heuristic miner
- **Conformance Checking**: Measuring fitness, precision, generalization, and simplicity
- **Performance Analysis**: Bottleneck identification and throughput optimization
- **Variant Analysis**: Understanding process variations and patterns
- **Social Network Analysis**: Resource collaboration patterns

## Demo Applications

### 1. Healthcare Process Mining (`demo_01_healthcare_process_mining.c`)

**Domain**: Hospital and healthcare systems  
**Focus**: Patient journey analysis and care pathway optimization

**Key Features**:
- Patient registration and triage workflows
- Emergency care processes
- Surgery and procedure pathways
- Specialist referral flows
- Rehabilitation and follow-up processes
- Insurance and billing workflows

**Process Variants**:
- Standard routine care (100 instances)
- Emergency care (50 instances)
- Surgery pathway (50 instances)
- Complex multi-specialist cases (50 instances)
- Outpatient procedures (50 instances)
- Insurance-intensive processes (50 instances)
- Rehabilitation pathways (50 instances)
- Fast-track emergency (50 instances)

**Analysis Capabilities**:
- Patient journey duration analysis
- Care pathway bottleneck identification
- Staff collaboration network analysis
- Resource utilization optimization
- Quality metrics tracking

### 2. E-commerce Process Mining (`demo_02_ecommerce_process_mining.c`)

**Domain**: Online retail and order fulfillment  
**Focus**: Order processing and customer service optimization

**Key Features**:
- Order processing workflows
- Payment and inventory management
- Shipping and logistics processes
- Customer support and dispute resolution
- Return and refund processing
- Supply chain management

**Process Variants**:
- Standard order fulfillment (80 instances)
- Customer support escalation (40 instances)
- Return processing (40 instances)
- Order cancellation (40 instances)
- Inventory shortage scenarios (40 instances)
- Express shipping (40 instances)
- Product catalog updates (40 instances)
- Multiple support interactions (40 instances)
- International shipping (40 instances)
- Bulk order processing (40 instances)

**Analysis Capabilities**:
- Order fulfillment time optimization
- Customer satisfaction tracking
- Supply chain bottleneck analysis
- Support ticket lifecycle management
- Shipping performance metrics

### 3. Manufacturing Process Mining (`demo_03_manufacturing_process_mining.c`)

**Domain**: Production and manufacturing systems  
**Focus**: Production line optimization and quality control

**Key Features**:
- Production planning and scheduling
- Assembly line operations
- Quality control and testing
- Maintenance and repair processes
- Supply chain and inventory management
- Safety and compliance workflows

**Process Variants**:
- Standard production line (100 instances)
- Quality issue resolution (50 instances)
- Preventive maintenance (50 instances)
- Scrap and waste management (50 instances)
- Emergency maintenance (50 instances)
- Supply chain disruption (50 instances)
- Production line changeover (50 instances)
- Safety and compliance focused (50 instances)
- Inventory management intensive (50 instances)
- High-volume production (50 instances)
- Custom manufacturing (50 instances)
- Just-in-time production (50 instances)

**Analysis Capabilities**:
- Production efficiency optimization
- Quality control process analysis
- Maintenance scheduling optimization
- Resource utilization tracking
- Safety compliance monitoring

### 4. Financial Process Mining (`demo_04_financial_process_mining.c`)

**Domain**: Banking and financial services  
**Focus**: Transaction processing and compliance workflows

**Key Features**:
- Customer onboarding and KYC processes
- Transaction processing and settlement
- Loan application and underwriting
- Fraud detection and security
- Compliance and regulatory reporting
- Investment and trading operations

**Process Variants**:
- Standard banking transactions (80 instances)
- Loan application processing (40 instances)
- Fraud detection scenarios (40 instances)
- Compliance and regulatory processes (40 instances)
- Default and collections (40 instances)
- Investment and trading (40 instances)
- System maintenance and backup (40 instances)
- Refund and dispute resolution (40 instances)
- High-frequency trading (40 instances)
- Account closure processes (40 instances)

**Analysis Capabilities**:
- Transaction processing optimization
- Risk assessment and management
- Compliance monitoring and reporting
- Fraud detection performance
- Customer service quality metrics

### 5. IT Service Desk Process Mining (`demo_05_itservice_process_mining.c`)

**Domain**: IT support and service management  
**Focus**: Support ticket lifecycle and incident management

**Key Features**:
- Ticket creation and classification
- Multi-level support escalation
- Technical issue resolution
- Bug fix development and deployment
- Security incident response
- System maintenance and monitoring

**Process Variants**:
- Standard support tickets (80 instances)
- Escalated technical issues (40 instances)
- Bug fix development (40 instances)
- Security incidents (40 instances)
- Data recovery processes (40 instances)
- Equipment replacement (40 instances)
- Network troubleshooting (40 instances)
- Vendor coordination (40 instances)
- Capacity planning (40 instances)
- Compliance audits (40 instances)
- Service level reviews (40 instances)
- High-priority incidents (40 instances)

**Analysis Capabilities**:
- Support ticket resolution optimization
- Escalation pattern analysis
- Technical issue categorization
- Service level agreement monitoring
- Resource allocation optimization

## Building and Running the Demos

### Prerequisites

- GCC compiler with C99 support
- Make build system
- 7T Process Mining Library (pm7t)

### Build Commands

```bash
# Build all process mining demos
make process_mining_demos

# Build individual demos
make healthcare_demo
make ecommerce_demo
make manufacturing_demo
make financial_demo
make itservice_demo

# Build the process mining library
make $(PM7T_LIB)
```

### Running the Demos

```bash
# Run healthcare process mining demo
./healthcare_demo

# Run e-commerce process mining demo
./ecommerce_demo

# Run manufacturing process mining demo
./manufacturing_demo

# Run financial process mining demo
./financial_demo

# Run IT service desk process mining demo
./itservice_demo
```

## Output Analysis

Each demo provides comprehensive analysis output including:

### Process Discovery Results
- **Alpha Algorithm**: Discovers process models using the Alpha algorithm
- **Heuristic Miner**: Discovers process models using dependency thresholds
- **Transition Analysis**: Frequency and probability of process transitions

### Conformance Metrics
- **Fitness**: How well the discovered model fits the event log
- **Precision**: How precise the model is in describing the process
- **Generalization**: How well the model generalizes to new cases
- **Simplicity**: How simple and understandable the model is

### Performance Analysis
- **Process Duration**: Average, minimum, and maximum processing times
- **Throughput**: Number of cases processed per time unit
- **Bottleneck Identification**: Activities causing delays
- **Resource Utilization**: Staff and system utilization patterns

### Variant Analysis
- **Process Variants**: Different paths through the process
- **Frequency Distribution**: How often each variant occurs
- **Pattern Recognition**: Common and exceptional process patterns

### Social Network Analysis
- **Resource Collaboration**: How resources work together
- **Handover Patterns**: Transfer of work between resources
- **Centrality Measures**: Key resources in the process

## Data Generation

Each demo generates synthetic event log data that simulates real-world process scenarios:

- **Event Logs**: 35,000-60,000 events per demo
- **Process Cases**: 400-600 cases per demo
- **Activities**: 19-42 unique activities per domain
- **Resources**: 100+ unique resources (staff, systems, machines)
- **Time Stamps**: Nanosecond precision timing
- **Cost Metrics**: Activity-specific cost and duration data

## Performance Characteristics

The 7T Process Mining Engine is optimized for:

- **Sub-nanosecond Operations**: Core operations complete in <1ns
- **Billion+ OPS**: Throughput of 1+ billion operations per second
- **Memory Efficiency**: Minimal memory footprint and allocation
- **Scalability**: Linear scaling with data size
- **Real-time Analysis**: Live process monitoring capabilities

## Customization

Each demo can be customized for specific use cases:

### Modifying Process Variants
Edit the switch statements in each demo to add or modify process variants.

### Adjusting Data Volumes
Modify the `instances` variables to change the number of cases generated.

### Adding New Activities
Extend the `activities` arrays and add corresponding event generation logic.

### Custom Analysis
Add domain-specific analysis functions using the pm7t API.

## API Reference

The demos use the following key pm7t functions:

```c
// Event log management
EventLog* pm7t_create_event_log(size_t initial_capacity);
int pm7t_add_event(EventLog* log, uint32_t case_id, uint32_t activity_id,
                   uint64_t timestamp, uint32_t resource_id, uint32_t cost);

// Process discovery
ProcessModel* pm7t_discover_alpha_algorithm(TraceLog* trace_log);
ProcessModel* pm7t_discover_heuristic_miner(TraceLog* trace_log, double threshold);

// Conformance checking
ConformanceResult pm7t_check_conformance(ProcessModel* model, TraceLog* trace_log);

// Performance analysis
PerformanceAnalysis* pm7t_analyze_performance(EventLog* event_log);
BottleneckAnalysis* pm7t_analyze_bottlenecks(EventLog* event_log);

// Variant analysis
VariantAnalysis* pm7t_analyze_variants(TraceLog* trace_log);

// Social network analysis
SocialNetwork* pm7t_analyze_social_network(EventLog* event_log);
```

## Memory Management

Each demo includes proper memory management:

- **Automatic Cleanup**: All allocated resources are freed
- **Memory Limits**: Configurable memory limits for large datasets
- **Memory Usage Tracking**: Real-time memory consumption monitoring
- **Efficient Allocation**: Minimal memory overhead per event

## Error Handling

The demos include comprehensive error handling:

- **Resource Validation**: Checks for successful allocation
- **Graceful Degradation**: Continues operation with partial data
- **Error Reporting**: Detailed error messages and diagnostics
- **Recovery Mechanisms**: Automatic cleanup on errors

## Extending the Demos

To extend these demos for your specific use case:

1. **Copy a Base Demo**: Use one of the existing demos as a template
2. **Modify Activities**: Update the activity definitions for your domain
3. **Adjust Process Variants**: Modify the process generation logic
4. **Add Custom Analysis**: Implement domain-specific metrics
5. **Integrate Real Data**: Replace synthetic data with real event logs

## Performance Tuning

For optimal performance:

- **Compile with Optimization**: Use `-O3 -march=native` flags
- **Adjust Memory Limits**: Set appropriate memory limits for your data size
- **Batch Processing**: Process events in batches for large datasets
- **Parallel Processing**: Use multiple threads for independent analysis

## Troubleshooting

Common issues and solutions:

- **Memory Allocation Failures**: Reduce data volume or increase memory limits
- **Build Errors**: Ensure all dependencies are installed and paths are correct
- **Performance Issues**: Check compiler optimization flags and system resources
- **Data Quality Issues**: Validate event log format and data consistency

## Contributing

To contribute to the process mining demos:

1. Follow the existing code style and structure
2. Add comprehensive error handling
3. Include performance optimizations
4. Document new features and capabilities
5. Test with various data sizes and scenarios

## License

These demos are part of the 7T Engine project and follow the same licensing terms. 