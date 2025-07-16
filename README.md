# AutoTel - Automated Telemetry and Semantic Execution Pipeline

AutoTel is an enterprise-grade BPMN 2.0 orchestration framework with AI-powered decision making, featuring comprehensive telemetry and semantic execution capabilities.

## Features

- **BPMN 2.0 Workflow Engine**: Full support for BPMN workflows with SpiffWorkflow
- **AI-Powered Decision Making**: Integration with DSPy for intelligent decision points
- **Comprehensive Telemetry**: OpenTelemetry integration with LinkML schema validation
- **Semantic Execution**: OWL/RDF ontology processing and compilation
- **Robust Fallback**: Graceful degradation when telemetry is unavailable
- **Enterprise Ready**: Production-grade error handling and validation

## Quick Start

### Installation

```bash
# Clone the repository
git clone <repository-url>
cd autotel

# Install dependencies using uv
uv sync

# Install the package
uv pip install -e .
```

### Basic Usage

```bash
# Show version and system information
autotel version

# Initialize AutoTel with configuration
autotel init

# Run a BPMN workflow
autotel run workflow.bpmn --input data.json

# Parse an OWL ontology
autotel ontology parse --file ontology.owl --export compiled.json
```

## Telemetry and Fallback

AutoTel includes a robust telemetry system with automatic fallback to no-op operations when telemetry is unavailable or disabled.

### Telemetry Modes

1. **Full Telemetry** (default): Complete OpenTelemetry integration with LinkML schema validation
2. **No-Telemetry Mode**: Disabled via `--no-telemetry` flag for environments where telemetry is not needed
3. **Automatic Fallback**: Graceful degradation to no-op operations if telemetry initialization fails

### Using No-Telemetry Mode

```bash
# Disable telemetry for all operations
autotel --no-telemetry ontology parse --file ontology.owl

# Run workflow without telemetry
autotel --no-telemetry run workflow.bpmn

# Initialize without telemetry
autotel --no-telemetry init
```

### Telemetry Validation

The system validates telemetry against LinkML schemas to ensure data quality:

```bash
# Show telemetry statistics
autotel telemetry --stats

# Export telemetry data
autotel telemetry --export traces.json --format json
```

## Architecture

### Core Components

- **Orchestrator**: Main workflow orchestration engine
- **Telemetry Manager**: OpenTelemetry integration with fallback support
- **Schema Validator**: LinkML-based validation system
- **OWL Processor**: Semantic ontology processing
- **Ontology Compiler**: Compilation of ontologies into executable schemas

### Pipeline Flow

1. **Input Processing**: BPMN/DMN/OWL files are parsed and validated
2. **Semantic Analysis**: Ontologies are processed and compiled
3. **Workflow Execution**: BPMN workflows are executed with telemetry
4. **Decision Making**: AI-powered decisions using DSPy integration
5. **Output Generation**: Results with comprehensive telemetry

### Telemetry Integration

```python
from autotel.core.telemetry import get_telemetry_manager_or_noop

# Always works, falls back to no-op if telemetry fails
telemetry = get_telemetry_manager_or_noop(
    service_name="my-service",
    require_linkml_validation=False
)

# Use telemetry with automatic fallback
with telemetry.start_span("operation", "processing") as span:
    span.set_attribute("input_size", len(data))
    # ... processing logic ...
```

## CLI Commands

### Core Commands

- `autotel version` - Show version and system information
- `autotel init` - Initialize AutoTel with configuration
- `autotel run <workflow>` - Execute a BPMN workflow
- `autotel validate <file>` - Validate BPMN/DMN/YAML files

### Telemetry Commands

- `autotel telemetry --stats` - Show telemetry statistics
- `autotel telemetry --export <file>` - Export telemetry data
- `autotel --no-telemetry <command>` - Run any command without telemetry

### Ontology Commands

- `autotel ontology parse --file <owl>` - Parse OWL ontology
- `autotel ontology parse --export <json>` - Export compiled ontology

### DSPy Commands

- `autotel dspy --list` - List available DSPy signatures
- `autotel dspy --call <signature>` - Call a DSPy signature

## Configuration

### Telemetry Configuration

```yaml
telemetry:
  enabled: true
  linkml_validation: true
  export_format: json
  schema_path: otel_traces_schema.yaml
```

### DSPy Configuration

```yaml
dspy:
  cache_enabled: true
  models:
    - openai:gpt-4o-mini
    - ollama:qwen2.5:7b
```

## Development

### Running Tests

```bash
# Run all tests
uv run pytest

# Run specific test categories
uv run pytest tests/test_telemetry.py
uv run pytest tests/test_owl_processor.py

# Run 80/20 validation tests
uv run python test_80_20_telemetry_validation.py
```

### Adding New Processors

1. Create processor in `autotel/factory/processors/`
2. Implement telemetry integration with fallback
3. Add CLI commands in `autotel/cli.py`
4. Write comprehensive tests

### Telemetry Best Practices

1. **Always use fallback**: Use `get_telemetry_manager_or_noop()` instead of direct instantiation
2. **Graceful degradation**: Ensure operations work without telemetry
3. **Schema validation**: Use LinkML schemas for telemetry data validation
4. **Error handling**: Capture failures in telemetry, don't let telemetry cause failures

## Troubleshooting

### Telemetry Issues

If telemetry is causing problems:

```bash
# Disable telemetry globally
autotel --no-telemetry <command>

# Check telemetry status
autotel telemetry --stats

# Export telemetry for debugging
autotel telemetry --export debug.json
```

### Common Issues

1. **LinkML schema not found**: Ensure `otel_traces_schema.yaml` is in the project root
2. **OpenTelemetry initialization fails**: Use `--no-telemetry` flag or check dependencies
3. **OWL parsing errors**: Validate OWL file format and namespace declarations

## Contributing

1. Fork the repository
2. Create a feature branch
3. Implement changes with telemetry integration
4. Add tests for both telemetry and no-telemetry modes
5. Submit a pull request

## License

[License information]

## Support

For support and questions:
- [Documentation](link-to-docs)
- [Issues](link-to-issues)
- [Discussions](link-to-discussions)