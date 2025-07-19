# CNS - CHATMAN NANO-STACK

A 7-tick performance-guaranteed CLI tool implementing the CHATMAN NANO-STACK architecture.

## Features

- **7-tick constraint**: All critical operations complete within 7 CPU cycles
- **Typer-like CLI**: Intuitive domain-verb command structure
- **Pure C implementation**: Zero dependencies, maximum performance
- **Six Sigma quality**: Built-in quality metrics and validation

## Installation

```bash
# Clone the repository
git clone https://github.com/seanchatman/cns.git
cd cns

# Build with make
make

# Install to /usr/local/bin
make install
```

## Usage

CNS uses a domain-verb pattern for all commands:

```bash
cns <domain> <verb> [options] [arguments]
```

### Examples

```bash
# Build commands
cns build init      # Initialize build system
cns build all       # Build all targets
cns build clean     # Clean build directory

# Test commands
cns test all        # Run all tests
cns test unit       # Run unit tests
cns test coverage   # Generate coverage report

# Benchmark commands
cns bench all       # Run all benchmarks
cns bench sparql    # Run SPARQL benchmarks
cns bench memory    # Run memory benchmarks

# Gatekeeper validation
cns gate run        # Run full CTQ validation
cns gate check 2    # Check specific CTQ

# Parse ontology
cns parse ontology/subprocess.ttl

# Dashboard
cns dashboard up    # Start monitoring dashboard
cns dashboard down  # Stop dashboard

# Documentation
cns docs validate   # Validate documentation
cns docs generate   # Generate docs
cns docs serve      # Serve docs locally

# Release management
cns release patch   # Create patch release
cns release minor   # Create minor release
cns release major   # Create major release

# Profiling
cns profile sparql7t_core -d 30  # Profile for 30 seconds

# Deployment
cns deploy --target homebrew     # Deploy to Homebrew
cns deploy --target docker       # Deploy as Docker image

# Six Sigma metrics
cns sigma calc -d 7 -o 1000000   # Calculate sigma level
cns sigma report                 # Generate quality report
```

## Domain Reference

| Domain | Description | Commands |
|--------|-------------|----------|
| `build` | Build system management | init, all, clean, debug |
| `test` | Testing framework | all, unit, coverage |
| `bench` | Performance benchmarking | all, sparql, memory |
| `gate` | Gatekeeper validation | run, check |
| `parse` | Ontology parsing | (default) |
| `dashboard` | Monitoring dashboard | up, down, status |
| `docs` | Documentation | validate, generate, serve |
| `release` | Release management | patch, minor, major |
| `profile` | Performance profiling | (default) |
| `deploy` | Package deployment | (default) |
| `sigma` | Six Sigma metrics | calc, report |

## 7-Tick Constraint

CNS enforces a strict 7-tick (CPU cycle) constraint on all critical operations. Any command that violates this constraint will fail with a `CNS_ERROR_CYCLES` error.

## Development

```bash
# Debug build
make clean
CFLAGS="-g -O0" make

# Run with profiling
cns profile sparql7t_core

# Check performance
cns gate run
```

## License

MIT License - See LICENSE file for details.