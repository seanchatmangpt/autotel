# CNS Weaver Documentation

The CNS Weaver is an automated code generation system that creates OpenTelemetry instrumentation from TTL ontology definitions. This directory contains comprehensive documentation for the weaver system.

## 📚 Documentation Index

### Core Documentation
- **[WEAVER.md](../WEAVER.md)** - Complete weaver system overview and usage guide
- **[validate_weaver.py](../validate_weaver.py)** - Comprehensive validation script

### Quick Start
- **[Getting Started](getting-started.md)** - Step-by-step setup guide
- **[Examples](examples.md)** - Practical usage examples
- **[Troubleshooting](troubleshooting.md)** - Common issues and solutions

### Technical Reference
- **[Architecture](architecture.md)** - System design and components
- **[API Reference](api-reference.md)** - Detailed CLI and API documentation
- **[Templates](templates.md)** - Jinja template reference
- **[Ontology](ontology.md)** - TTL ontology structure and format

### Integration
- **[Build Integration](build-integration.md)** - CMake and build system integration
- **[Gatekeeper Integration](gatekeeper-integration.md)** - Integration with CNS Gatekeeper
- **[Performance](performance.md)** - Performance tracking and Six Sigma validation

## 🚀 Quick Start

```bash
# Extract spans from TTL ontology
python3 codegen/extract_spans.py docs/ontology/cns-core.ttl build/cns_spans.json

# Generate OpenTelemetry code
python3 codegen/weaver_simple.py build/cns_spans.json templates/otel_header.h.j2 templates/otel_inject.c.j2 src cns.h

# Validate the system
python3 docs/validate_weaver.py
```

## 🎯 Key Features

- **Automatic Code Generation**: Generate OpenTelemetry instrumentation from TTL
- **Performance Tracking**: Built-in cycle counting and violation detection
- **Six Sigma Integration**: Automatic sigma calculation and ppm tracking
- **Pattern Support**: Macro definitions for design patterns
- **Gatekeeper Integration**: Seamless integration with CNS validation
- **Zero Manual Editing**: Everything generated from ontology

## 📊 System Components

1. **Ontology Definition** (`docs/ontology/cns-core.ttl`)
2. **Span Extractor** (`codegen/extract_spans.py`)
3. **Weaver CLI** (`codegen/weaver_simple.py`)
4. **Jinja Templates** (`templates/`)
5. **Validation Script** (`docs/validate_weaver.py`)

## 🔧 CLI Tools

- **Span Extractor**: `python3 codegen/extract_spans.py <ttl_file> <output_json>`
- **Weaver CLI**: `python3 codegen/weaver_simple.py <ctx.json> <header_tmpl> <inject_tmpl> <out_dir> [inject_target_header]`
- **Validation**: `python3 docs/validate_weaver.py`

## 📈 Generated Output

- **Header File** (`src/cns_otel.h`): OpenTelemetry stubs and performance tracking
- **Inject File** (`src/cns_otel_inject.c`): Instrumented function wrappers
- **JSON Context** (`build/cns_spans.json`): Extracted spans and functions

## 🎉 Benefits

1. **Consistency**: Every function gets proper telemetry instrumentation
2. **Performance**: Built-in Six Sigma compliance validation
3. **Maintainability**: Changes to TTL automatically propagate to code
4. **Integration**: Seamless integration with existing CNS systems
5. **Automation**: Zero manual editing required

## 📞 Support

For questions or issues with the weaver system:
1. Check the [Troubleshooting](troubleshooting.md) guide
2. Review the [Examples](examples.md) for usage patterns
3. Run the [validation script](../validate_weaver.py) to diagnose issues
4. Consult the [API Reference](api-reference.md) for detailed documentation 