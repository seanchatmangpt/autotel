# AutoTel - Enterprise BPMN 2.0 Orchestration Framework

AutoTel is an enterprise-grade **BPMN 2.0 orchestration framework** that combines workflow automation with AI-powered decision making, semantic validation, and comprehensive telemetry. It integrates seven key pillars, all implemented as unified processors:

- **BPMN 2.0** - Business process modeling and execution
- **DMN** - Decision Model and Notation for business rules
- **DSPy** - AI-powered intelligent services (XML-only configuration, no hardcoded Python config)
- **SHACL** - Data validation and constraints (replaces LinkML for validation)
- **OWL** - Semantic ontology processing
- **Jinja2** - Dynamic template processing (now with unified JinjaProcessor)
- **OpenTelemetry** - Comprehensive observability
- **AnyStore** - Universal data persistence layer supporting multiple formats

## Unified Processor Architecture

All core processors (BPMN, DMN, DSPy, SHACL, OWL, Jinja2, OTEL) follow a unified architecture:
- **Type-safe Pydantic models** for config, results, and data
- **Telemetry-first**: All operations are traced and logged using OpenTelemetry (or a no-op fallback)
- **Contract-based programming**: Pre/postconditions for all processor entry points
- **80/20 happy-path focus**: Processors implement the most common use cases robustly, letting rare errors crash (let it crash philosophy)
- **Comprehensive error handling and logging**
- **XML-first**: All configuration and definitions are in XML (especially for DSPy)
- **No LinkML for validation**: SHACL and OWL are now used exclusively for schema and data validation

## Project Structure

```
autotel/
├── core/           # Framework, orchestrator, telemetry
├── factory/        # Processor factories, dynamic processor generation
├── helpers/        # Contracts, error handling, telemetry helpers
├── processors/     # Unified processor implementations (bpmn, dmn, dspy, shacl, owl, jinja, otel)
├── schemas/        # Pydantic models for processor data
├── stores/         # Data persistence layer (AnyStore, JSONStore, YAMLStore, etc.)
├── utils/          # Utility functions
├── workflows/      # Workflow engine integrations
└── autotel_cli.py  # Typer-based CLI
```

## Features

- **Unified Processor Pattern**: All processors share a common interface, config, and telemetry
- **BPMN 2.0 Orchestration**: Complete workflow automation with SpiffWorkflow engine (FAANG-level implementation)
- **DMN Integration**: Business rule execution with decision tables
- **AI-Powered Services**: DSPy integration for intelligent task execution (XML-only)
- **Semantic Validation**: SHACL constraint validation and OWL ontology processing
- **Dynamic Templating**: Jinja2 integration for dynamic content generation (via JinjaProcessor)
- **Comprehensive Telemetry**: OpenTelemetry integration (telemetry is the only source of truth for system behavior)
- **Universal Data Persistence**: AnyStore supports JSON, YAML, Pickle, XML, Parquet, and SQLite formats
- **Factory Boy Integration**: All processors and test data are generated and validated using Factory Boy
- **80/20 Rule**: Focus on robust happy-path implementation, let rare errors crash
- **Enterprise Ready**: Production-grade error handling and validation
- **Pydantic v2 Ready**: Enhanced type safety and performance

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

# Use AnyStore for data persistence
autotel store save data.json --format yaml --output data.yaml
autotel store load data.yaml --format yaml
```

## Architecture

### Seven Pillars Integration

AutoTel integrates seven key technologies into a unified orchestration framework, each as a unified processor:

1. **BPMN 2.0** - Business process modeling and execution using SpiffWorkflow
2. **DMN** - Decision Model and Notation for business rule execution
3. **DSPy** - AI-powered intelligent services with dynamic signatures (XML-only)
4. **SHACL** - Data validation and constraint checking (no LinkML)
5. **OWL** - Semantic ontology processing and reasoning
6. **Jinja2** - Dynamic template processing and content generation (unified JinjaProcessor)
7. **OpenTelemetry** - Comprehensive observability and monitoring

### Data Persistence Layer

AutoTel includes a universal data persistence layer with **AnyStore**:

- **Format Detection**: Automatically detects file format by extension
- **Multiple Formats**: JSON, YAML, Pickle, XML, Parquet, SQLite
- **Unified Interface**: Single API for all storage operations
- **Telemetry Integration**: All storage operations are traced
- **Error Handling**: Graceful fallback and detailed error reporting
- **Metadata Enrichment**: Automatic metadata and timestamp tracking

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
    ↓
AnyStore → Data Persistence → Multi-format Storage
```

### Core Components

- **Workflow Engine**: SpiffWorkflow-based BPMN execution engine (FAANG-level)
- **DMN Engine**: Decision table execution and business rule processing
- **DSPy Integration**: AI service execution with dynamic signatures (XML-only)
- **Telemetry Manager**: OpenTelemetry integration with fallback support
- **OWL Processor**: Semantic ontology processing (unified)
- **SHACL Processor**: Constraint validation processing (unified, replaces LinkML)
- **JinjaProcessor**: Dynamic template processing and rendering (unified)
- **AnyStore**: Universal data persistence with format detection
- **Factory Integration**: All test data and configs generated with Factory Boy

## CLI Commands

### Core Commands

- `autotel version` - Show version and system information
- `autotel init` - Initialize AutoTel with configuration
- `autotel run <workflow.bpmn>` - Run a BPMN workflow
- `autotel list --workflows` - List available workflows
- `autotel validate <file>` - Validate BPMN/DMN/OWL/SHACL/DSPy files
- `autotel workflow --validate <file>` - Validate workflow structure

### Data Persistence Commands

- `autotel store save <file> --format <format>` - Save data in specified format
- `autotel store load <file> --format <format>` - Load data from specified format
- `autotel store convert <input> --from <format> --to <format>` - Convert between formats
- `autotel store info <file>` - Show file format and metadata information

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
- `autotel dspy --call <signature>` - Call a DSPy signature (XML-only config)

### Jinja2 Commands

- `autotel jinja --render <template.xml>` - Render Jinja2 template
- `autotel jinja --validate <template.xml>` - Validate Jinja2 template
- `autotel jinja --process <template.xml> --context <data.json>` - Process template with context

## Testing & Development

### Running Tests

```bash
# Run all tests
uv run pytest

# Run integration tests (including all processors and Factory Boy data)
uv run python test_all_processors_integration.py
uv run python test_pipeline.py

# Run dynamic workflow tests
uv run python test_dynamic_dspy_jinja_bpmn.py

# Run AnyStore tests
uv run python test_anystore.py
```

### Best Practices

- **Telemetry is the only source of truth**: All claims of feature implementation or correctness must be validated against actual telemetry output (spans, events, traces)
- **80/20 Rule**: Focus on robust happy-path implementation, let rare errors crash
- **Factory Boy**: All test data and configs are generated with Factory Boy, no hardcoded values
- **XML-Only DSPy**: DSPy configuration and definitions are always in XML, never in Python code
- **No LinkML**: SHACL and OWL are used for all validation
- **Unified Processor Pattern**: All processors follow the same interface, config, and telemetry pattern
- **Data Persistence**: Use AnyStore for all data storage operations with automatic format detection

### Adding New Processors

1. Create processor in `autotel/processors/` (not `autotel/factory/processors/`)
2. Implement telemetry integration with fallback
3. Add CLI commands in `autotel_cli.py`
4. Write comprehensive tests with Factory Boy integration
5. Follow Pydantic v2 migration guidelines (see `PYDANTIC-V2-PROCESSOR-REFACTOR.md`)

### Telemetry Best Practices

1. **Always use fallback**: Use `get_telemetry_manager_or_noop()` instead of direct instantiation
2. **Graceful degradation**: Ensure operations work without telemetry
3. **Schema validation**: Use SHACL/OWL schemas for validation (no LinkML)
4. **Error handling**: Capture failures in telemetry, don't let telemetry cause failures

## Recent Updates

### Latest Features
- **AnyStore Universal Data Persistence**: Multi-format data storage with automatic format detection (JSON, YAML, Pickle, XML, Parquet, SQLite)
- **FAANG-Level BPMN Processor**: Enhanced BPMN processor with unified telemetry, robust error handling, and best-practice metadata registration
- **Unified SPARQL Processor**: SPARQL processor with factoryboy integration and comprehensive tests
- **Comprehensive Documentation**: Complete AnyStore documentation with quick reference guides and implementation details
- **Unified JinjaProcessor**: Dynamic template processing with XML configuration, telemetry, and contract-based programming
- **Factory Boy Integration**: All processors and test data are generated and validated using Factory Boy
- **SHACL/OWL Validation**: All validation now uses SHACL and OWL (no LinkML)
- **80/20 Happy Path**: All processors focus on robust happy-path implementation, let rare errors crash
- **Pydantic v2 Migration Plan**: Comprehensive refactor strategy for enhanced performance

### Performance Improvements
- **Unified Processor Pattern**: All processors now follow a unified architecture
- **Test Coverage**: Enhanced integration testing with real processor methods and Factory Boy data
- **Error Handling**: Improved validation and error reporting
- **Telemetry**: Better span tracking and metrics collection
- **Data Persistence**: Optimized storage operations with format-specific backends

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

### Data Persistence Issues

If AnyStore operations fail:

```bash
# Check file format
autotel store info <file>

# Convert between formats
autotel store convert <input> --from <format> --to <format>

# Validate file structure
autotel validate <file>
```

### Common Issues

1. **SHACL/OWL schema not found**: Ensure your schema files are in the project root
2. **OpenTelemetry initialization fails**: Use `--no-telemetry` flag or check dependencies
3. **OWL parsing errors**: Validate OWL file format and namespace declarations
4. **DMN parsing issues**: Check XML namespace format (use default namespace, not prefixed)
5. **Jinja2 template errors**: Validate XML structure and CDATA sections
6. **AnyStore format detection fails**: Check file extension or specify format explicitly

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
- **Data Persistence**: Use AnyStore for all storage operations with proper format detection

## License

[License information]

## Support

For support and questions:
- [Documentation](link-to-docs)
- [Issues](link-to-issues)
- [Discussions](link-to-discussions)
