# AutoTel Prototype

An enterprise-grade BPMN 2.0 orchestration engine with zero-touch telemetry integration, powered by SpiffWorkflow.

## 🚀 Features

- **Full BPMN 2.0 Compliance**: Complete support for BPMN 2.0 specification
- **Zero-Touch Telemetry**: Built-in OpenTelemetry integration for observability
- **Process Persistence**: State management and recovery capabilities
- **Advanced Monitoring**: Real-time process and task execution tracking
- **Multi-Instance Support**: Handle multiple concurrent process instances
- **Event-Driven Execution**: Reactive process execution based on events
- **Error Handling**: Comprehensive error handling and recovery mechanisms

## 🏗️ Architecture

The AutoTel prototype consists of several key components:

- **BPMN Orchestrator**: Core workflow execution engine
- **Telemetry Integration**: OpenTelemetry-based observability
- **Process Management**: Instance lifecycle and state management
- **Task Execution**: Individual task processing and monitoring

## 📋 Prerequisites

- Python 3.11+
- Virtual environment (recommended)
- Git

## 🛠️ Installation

1. **Clone the repository**:
   ```bash
   git clone <repository-url>
   cd autotel_prototype
   ```

2. **Create and activate virtual environment**:
   ```bash
   python -m venv autotel_env
   source autotel_env/bin/activate  # On Windows: autotel_env\Scripts\activate
   ```

3. **Install dependencies**:
   ```bash
   pip install -r requirements.txt
   ```

## 🚀 Quick Start

1. **Run the orchestrator**:
   ```bash
   python bpmn_orchestrator.py
   ```

2. **Example usage**:
   ```python
   from bpmn_orchestrator import BPMNOrchestrator
   
   # Initialize orchestrator
   orchestrator = BPMNOrchestrator(bpmn_files_path="bpmn")
   
   # Start a process
   instance = orchestrator.start_process("Process_1", {"input": "test"})
   
   # Execute the process
   result = orchestrator.execute_process(instance.instance_id)
   print(f"Process status: {result.status.value}")
   ```

## 📁 Project Structure

```
autotel_prototype/
├── bpmn/                          # BPMN process definitions
│   ├── sample_process.bpmn        # Sample BPMN process
│   ├── simple_process.bpmn        # Simple workflow example
│   └── human_task_process.bpmn    # Human task workflow
├── bpmn_orchestrator.py           # Main orchestrator implementation
├── autotel_prototype.py           # Core AutoTel functionality
├── autotel_demo_ontology.yaml     # Demo ontology configuration
├── requirements.txt               # Python dependencies
├── README.md                      # This file
└── .gitignore                     # Git ignore rules
```

## 🔧 Configuration

### BPMN Process Definitions

Place your BPMN 2.0 XML files in the `bpmn/` directory. The orchestrator will automatically load all `.bpmn` files from this directory.

### Telemetry Configuration

The orchestrator includes built-in OpenTelemetry integration:

- **Tracing**: Process and task execution traces
- **Metrics**: Performance counters and histograms
- **Logging**: Structured logging with correlation IDs

### Environment Variables

Create a `.env` file for environment-specific configuration:

```env
# Telemetry
OTEL_EXPORTER_OTLP_ENDPOINT=http://localhost:4317
OTEL_SERVICE_NAME=autotel-orchestrator

# Logging
LOG_LEVEL=INFO

# Process Management
MAX_PROCESS_STEPS=100
PROCESS_CLEANUP_HOURS=24
```

## 📊 Monitoring and Observability

### Metrics

The orchestrator exposes the following metrics:

- `bpmn_processes_total`: Total number of processes executed
- `bpmn_task_duration_seconds`: Task execution duration histogram
- `bpmn_errors_total`: Total number of execution errors

### Traces

Each process and task execution is traced with:

- Process instance ID
- Task execution context
- Performance timing
- Error details

### Logs

Structured logging includes:

- Process lifecycle events
- Task execution details
- Error messages with context
- Performance metrics

## 🧪 Testing

Run the test suite:

```bash
# Run all tests
pytest

# Run with coverage
pytest --cov=bpmn_orchestrator

# Run specific test file
pytest test_bpmn_orchestrator.py
```

## 📈 Performance

The orchestrator is designed for enterprise-scale workloads:

- **Concurrent Processing**: Support for multiple process instances
- **Memory Efficient**: Optimized workflow state management
- **Fast Execution**: Efficient task scheduling and execution
- **Scalable**: Horizontal scaling capabilities

## 🔒 Security

- **Input Validation**: All process inputs are validated
- **Error Isolation**: Process failures don't affect other instances
- **Audit Trail**: Complete execution history tracking
- **Access Control**: Role-based access to process management

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🆘 Support

For support and questions:

- Create an issue in the repository
- Check the documentation
- Review the example BPMN processes

## 🔮 Roadmap

- [ ] Web-based process designer
- [ ] REST API for process management
- [ ] Advanced workflow patterns support
- [ ] Integration with external systems
- [ ] Real-time process monitoring dashboard
- [ ] Advanced error recovery mechanisms
- [ ] Performance optimization features

## 📚 References

- [BPMN 2.0 Specification](https://www.omg.org/spec/BPMN/2.0/)
- [SpiffWorkflow Documentation](https://spiffworkflow.readthedocs.io/)
- [OpenTelemetry Documentation](https://opentelemetry.io/docs/)
- [Python Best Practices](https://docs.python-guide.org/)

---

**AutoTel Prototype** - Enterprise BPMN Orchestration with Zero-Touch Telemetry
