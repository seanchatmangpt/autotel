# Connected Engines Demonstration

## Overview

This demonstration showcases the powerful integration between all 7T Framework engines:
- **PM7T** (Process Mining)
- **SPARQL7T** (Knowledge Graphs)
- **SHACL7T** (Validation)
- **MCTS7T** (Optimization)

## Running the Demo

### Quick Start
```bash
./run_demo.sh
```

### Direct Python Execution
```bash
python3 demo_connected_engines.py
```

## What the Demo Shows

### 1. Process Mining with PM7T
- Generates 1000 process instances with realistic data
- Discovers process models using Alpha algorithm
- Identifies bottlenecks and process variants
- Creates visual process flow diagrams

### 2. Knowledge Graph Construction with SPARQL7T
- Converts process models to RDF triples
- Enables semantic querying of process data
- Links activities, resources, and performance metrics
- Demonstrates SPARQL queries for insights

### 3. Business Rule Validation with SHACL7T
- Defines business constraints as SHACL shapes
- Validates process conformance
- Checks resource allocation rules
- Generates compliance reports

### 4. Process Optimization with MCTS7T
- Runs Monte Carlo Tree Search simulations
- Identifies optimization opportunities
- Suggests process improvements
- Calculates expected impact

### 5. Real-time Monitoring
- Simulates live event streaming
- Shows conformance checking in real-time
- Displays performance dashboard
- Alerts on violations and bottlenecks

## Demo Features

- **Beautiful Terminal UI**: Color-coded output with progress bars
- **Interactive Elements**: Real-time monitoring simulation
- **Comprehensive Reporting**: Final summary of all findings
- **Data Persistence**: Saves logs and results for analysis

## Expected Output

The demo will:
1. Generate sample process logs (saved to `demo_process_logs.json`)
2. Extract and visualize process models
3. Create knowledge graph representations
4. Validate against business rules
5. Find optimization opportunities
6. Demonstrate real-time monitoring
7. Generate a comprehensive final report

## Duration

The full demo takes approximately 2-3 minutes to complete.

## Requirements

- Python 3.6+
- No external dependencies (uses standard library only)
- Terminal with ANSI color support (most modern terminals)

## Customization

You can modify the demo by editing:
- Process types and activities
- Number of generated instances
- Business rules and constraints
- Optimization parameters
- Monitoring events

## Integration Points

The demo simulates the integration between engines. In a real deployment:
- PM7T would use the C implementation for performance
- SPARQL7T would connect to an RDF triplestore
- SHACL7T would load actual SHACL shapes
- MCTS7T would run the optimized C algorithms

This demonstration provides a compelling visual showcase of how all engines work together to provide end-to-end process intelligence.