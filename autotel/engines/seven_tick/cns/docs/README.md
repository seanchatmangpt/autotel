# CNS (Core Neural System) Documentation

## Overview

CNS is a high-performance, 7-tick optimized CLI framework that integrates multiple specialized engines for semantic computing, validation, templating, and telemetry. Built with performance-first principles, CNS provides sub-microsecond operations while maintaining a clean, domain-oriented interface.

## 🚀 Ported Subsystems

### SPARQL Engine
Ultra-fast triple pattern matching and knowledge graph operations with SIMD optimizations.

**Key Features:**
- 7-tick performance guarantee
- Cache-friendly memory layout
- SIMD-optimized batch operations
- Pattern matching with wildcards

**Commands:**
```bash
cns sparql query <pattern>     # Execute SPARQL queries
cns sparql add <triple>        # Add triples to graph
cns sparql benchmark           # Performance benchmarks
cns sparql test               # Unit tests
```

### SHACL Engine
High-performance shape validation and constraint checking for semantic data.

**Key Features:**
- Shape-based validation
- Constraint checking
- 7-tick performance optimization
- Telemetry integration

**Commands:**
```bash
cns shacl validate <data> <shapes>  # Validate data against shapes
cns shacl check <constraints>       # Check constraints
cns shacl benchmark                 # Performance benchmarks
cns shacl test                     # Unit tests
```

### CJinja Engine
Sub-microsecond template rendering with support for variables, conditionals, loops, and filters.

**Key Features:**
- Variable substitution
- Conditional rendering
- Loop support
- Custom filters
- 7-tick performance

**Commands:**
```bash
cns cjinja render <template> <vars>  # Render templates
cns cjinja compile <template>        # Compile templates
cns cjinja benchmark                 # Performance benchmarks
cns cjinja test                     # Unit tests
```

### Telemetry Engine
OpenTelemetry-like distributed tracing and performance metrics with 7-tick optimization.

**Key Features:**
- Span management
- Performance metrics
- Distributed tracing
- Export capabilities

**Commands:**
```bash
cns telemetry start                  # Start telemetry collection
cns telemetry stop                   # Stop telemetry collection
cns telemetry report                 # Generate performance reports
cns telemetry export                 # Export telemetry data
cns telemetry benchmark              # Telemetry performance tests
cns telemetry example                # Example usage
```

## 🏗️ Architecture

### Domain-Command Structure
CNS uses a domain-verb command structure where each subsystem is a domain with specific commands:

```
cns <domain> <command> [options] [arguments]
```

### Performance Guarantees
All operations are optimized for 7 CPU cycles or less, ensuring sub-microsecond performance for critical operations.

### Cross-Platform Support
- x86_64 with AVX2/SIMD optimizations
- ARM64 (Apple Silicon) compatibility
- Conditional compilation for architecture-specific features

## 📚 Documentation Structure

- **Getting Started**: `getting-started/` - Quick start guides and tutorials
- **Architecture**: `architecture/` - System design and architecture docs
- **API Reference**: `api/` - Detailed API documentation
- **Commands**: `commands/` - Command reference and examples
- **Performance**: `performance/` - Performance tuning and optimization
- **Integration**: `integration/` - Integration guides and examples
- **Testing**: `testing/` - Testing strategies and examples
- **Debugging**: `debugging/` - Troubleshooting and debugging guides
- **Best Practices**: `best-practices/` - Development and usage guidelines
- **FAQ**: `faq/` - Frequently asked questions

## 🚀 Quick Start

### Installation
```bash
cd cns
make clean
make OTEL_ENABLED=0  # Build without OpenTelemetry
```

### Basic Usage
```bash
# List available domains
./cns help

# Run SPARQL benchmark
./cns sparql benchmark

# Validate data with SHACL
./cns shacl validate data.ttl shapes.ttl

# Render template with CJinja
./cns cjinja render "Hello {{name}}!" name=World

# Start telemetry collection
./cns telemetry start
```

## 🔧 Development

### Building
```bash
# Standard build
make

# Build with OpenTelemetry
make OTEL_ENABLED=1

# Debug build
make debug

# Profile build
make profile
```

### Testing
```bash
# Run all tests
make test

# Run specific domain tests
./cns sparql test
./cns shacl test
./cns cjinja test
./cns telemetry test
```

### Benchmarking
```bash
# Run all benchmarks
make bench

# Run specific benchmarks
./cns sparql benchmark
./cns shacl benchmark
./cns cjinja benchmark
./cns telemetry benchmark
```

## 📊 Performance

All subsystems are designed to meet the 7-tick performance constraint:

- **SPARQL**: Sub-microsecond triple pattern matching
- **SHACL**: Ultra-fast shape validation
- **CJinja**: Sub-microsecond template rendering
- **Telemetry**: Minimal overhead tracing

## 🤝 Contributing

1. Follow the 7-tick performance constraint
2. Maintain cross-platform compatibility
3. Add comprehensive tests
4. Update documentation
5. Use telemetry for performance monitoring

## 📄 License

This project is part of the 7-tick optimization framework.

## 🔗 Related Documentation

- [Architecture Overview](architecture/README.md)
- [API Reference](api/README.md)
- [Performance Guide](performance/README.md)
- [Integration Guide](integration/README.md)
- [Testing Guide](testing/README.md) 