# AutoTel - Enterprise BPMN 2.0 Orchestration Framework

AutoTel is an enterprise-grade **BPMN 2.0 orchestration framework** that combines workflow automation with AI-powered decision making, semantic validation, and comprehensive telemetry. It integrates seven key pillars:

- **BPMN 2.0** - Business process modeling and execution
- **DMN** - Decision Model and Notation for business rules
- **DSPy** - AI-powered intelligent services
- **SHACL** - Data validation and constraints
- **OWL** - Semantic ontology processing
- **Jinja2** - Dynamic template processing
- **OpenTelemetry** - Comprehensive observability

## Features

- **BPMN 2.0 Orchestration**: Complete workflow automation with SpiffWorkflow engine
- **DMN Integration**: Business rule execution with decision tables
- **AI-Powered Services**: DSPy integration for intelligent task execution
- **Semantic Validation**: SHACL constraint validation and OWL ontology processing
- **Dynamic Templating**: Jinja2 integration for dynamic content generation
- **Comprehensive Telemetry**: OpenTelemetry integration with LinkML schema validation
- **XML-First Configuration**: All definitions in XML with no embedded code
- **Factory Boy Integration**: Robust test data generation and validation
- **Enterprise Ready**: Production-grade error handling and validation
- **Pydantic v2 Ready**: Migration plan for enhanced type safety and performance

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
autotel run bpmn/simple_process.bpmn --input '{"data": "test"}'

# List available workflows
autotel list --workflows

# Validate BPMN workflow
autotel validate bpmn/simple_process.bpmn

# Execute semantic pipeline
autotel pipeline execute --owl ontology.owl --shacl shapes.shacl --dspy signatures.dspy --input data.json
```

## Architecture

### Seven Pillars Integration

AutoTel integrates seven key technologies into a unified orchestration framework:

1. **BPMN 2.0** - Business process modeling and execution using SpiffWorkflow
2. **DMN** - Decision Model and Notation for business rule execution
3. **DSPy** - AI-powered intelligent services with dynamic signatures
4. **SHACL** - Data validation and constraint checking
5. **OWL** - Semantic ontology processing and reasoning
6. **Jinja2** - Dynamic template processing and content generation
7. **OpenTelemetry** - Comprehensive observability and monitoring

### Workflow Execution Pipeline

```
BPMN XML → Parser → Workflow Engine → Task Execution → Results + Telemetry
    ↓
DMN XML → Decision Engine → Rule Evaluation
    ↓
DSPy XML → Signature Registry → AI Service Execution
    ↓
SHACL XML → Validation Engine → Data Validation
    ↓
OWL XML → Ontology Engine → Semantic Processing
    ↓
Jinja2 XML → Template Engine → Dynamic Content Generation
    ↓
OTEL XML → Telemetry Engine → Observability & Monitoring
```

### Core Components

- **Workflow Engine**: SpiffWorkflow-based BPMN execution engine
- **DMN Engine**: Decision table execution and business rule processing
- **DSPy Integration**: AI service execution with dynamic signatures
- **Telemetry Manager**: OpenTelemetry integration with fallback support
- **Schema Validator**: LinkML-based validation system
- **OWL Processor**: Semantic ontology processing
- **SHACL Processor**: Constraint validation processing
- **Jinja2 Processor**: Dynamic template processing and rendering
- **Factory Integration**: Test data generation and validation

## CLI Commands

### Core Commands

- `autotel version` - Show version and system information
- `autotel init` - Initialize AutoTel with configuration
- `autotel run <workflow.bpmn>` - Run a BPMN workflow
- `autotel list --workflows` - List available workflows
- `autotel validate <file>` - Validate BPMN/DMN/OWL/SHACL/DSPy files
- `autotel workflow --validate <file>` - Validate workflow structure

### Telemetry Commands

- `autotel telemetry --stats` - Show telemetry statistics
- `autotel telemetry --export <file>` - Export telemetry data
- `autotel --no-telemetry <command>` - Run any command without telemetry

### Workflow Commands

- `autotel run <workflow.bpmn> --input <json>` - Run workflow with input data
- `autotel run <workflow.bpmn> --input-file <file.json>` - Run workflow with input file
- `autotel run <workflow.bpmn> --dmn <decision.dmn>` - Run workflow with DMN decisions
- `autotel list --processes` - List processes in workflow files
- `autotel list --dmn` - List DMN decisions
- `autotel workflow --info <file>` - Show workflow information

### DSPy Commands

- `autotel dspy --list` - List available DSPy signatures
- `autotel dspy --call <signature>` - Call a DSPy signature

### Jinja2 Commands

- `autotel jinja --render <template.xml>` - Render Jinja2 template
- `autotel jinja --validate <template.xml>` - Validate Jinja2 template
- `autotel jinja --process <template.xml> --context <data.json>` - Process template with context

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
uv run pytest tests/test_factory_boy_integration.py

# Run integration tests
uv run python test_all_processors_integration.py
uv run python test_pipeline.py

# Run dynamic workflow tests
uv run python test_dynamic_dspy_jinja_bpmn.py
```

### Adding New Processors

1. Create processor in `autotel/factory/processors/`
2. Implement telemetry integration with fallback
3. Add CLI commands in `autotel/cli.py`
4. Write comprehensive tests with Factory Boy integration
5. Follow Pydantic v2 migration guidelines (see `PYDANTIC-V2-PROCESSOR-REFACTOR.md`)

### Telemetry Best Practices

1. **Always use fallback**: Use `get_telemetry_manager_or_noop()` instead of direct instantiation
2. **Graceful degradation**: Ensure operations work without telemetry
3. **Schema validation**: Use LinkML schemas for telemetry data validation
4. **Error handling**: Capture failures in telemetry, don't let telemetry cause failures

## Recent Updates

### Latest Features
- **Jinja2 Integration**: Dynamic template processing with XML configuration
- **Factory Boy Integration**: Robust test data generation and validation
- **Enhanced DMN Processing**: Improved SpiffWorkflow integration
- **Dynamic Workflow Generation**: DSPy + Jinja2 → BPMN → SpiffWorkflow pipeline
- **Pydantic v2 Migration Plan**: Comprehensive refactor strategy for enhanced performance

### Performance Improvements
- **DMN Processor**: Fixed API integration with SpiffWorkflow
- **Test Coverage**: Enhanced integration testing with real processor methods
- **Error Handling**: Improved validation and error reporting
- **Telemetry**: Better span tracking and metrics collection

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
4. **DMN parsing issues**: Check XML namespace format (use default namespace, not prefixed)
5. **Jinja2 template errors**: Validate XML structure and CDATA sections

## Contributing

1. Fork the repository
2. Create a feature branch
3. Implement changes with telemetry integration
4. Add tests with Factory Boy integration for robust data generation
5. Follow Pydantic v2 migration guidelines for new processors
6. Ensure backward compatibility and graceful degradation
7. Submit a pull request

### Development Guidelines

- **Telemetry First**: All operations should work with and without telemetry
- **Factory Boy**: Use Factory Boy for test data generation instead of hardcoded values
- **Type Safety**: Follow Pydantic v2 patterns for data validation
- **Error Handling**: Implement graceful error handling with detailed telemetry
- **Testing**: Write comprehensive integration tests for all processor combinations

## License

[License information]

## Support

For support and questions:
- [Documentation](link-to-docs)
- [Issues](link-to-issues)
- [Discussions](link-to-discussions)
