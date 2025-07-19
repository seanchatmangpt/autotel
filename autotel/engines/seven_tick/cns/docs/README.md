# CNS (Command Nano Stack) Documentation

## Overview

CNS (Command Nano Stack) is a high-performance, 7-tick compliant command processing system designed for ultra-low latency applications. It provides a complete command parsing, dispatch, and execution framework optimized for sub-microsecond performance.

## Architecture

### Core Components

```
CNS System Architecture
├── Core Engine (cns.h)
│   ├── Command Registration
│   ├── Hash-based Lookup
│   ├── Execution Engine
│   └── Performance Tracking
├── Parser (cns_parser.h)
│   ├── Tokenization
│   ├── Quote Handling
│   ├── Argument Extraction
│   └── Validation
├── Dispatch (cns_dispatch.h)
│   ├── Command Routing
│   ├── Batch Execution
│   ├── Metrics Collection
│   └── Help System
├── Commands (cns_commands.h)
│   ├── Built-in Handlers
│   ├── Registration Macros
│   ├── Help Commands
│   └── Debug Tools
├── Benchmark (cns_benchmark.h)
│   ├── Performance Testing
│   ├── Tier Classification
│   ├── Statistical Analysis
│   └── Target Validation
├── Types (cns/types.h)
│   ├── Option Definitions
│   ├── Argument Structures
│   ├── Command Contexts
│   └── Result Codes
└── CLI (cns/cli.h)
    ├── Domain Management
    ├── Argument Parsing
    ├── Help System
    └── Error Handling
```

## Performance Targets

### 7-Tick Compliance
- **L1 Tier**: < 10ns (7-tick target)
- **L2 Tier**: < 100ns (sub-100ns)
- **L3 Tier**: < 1μs (sub-microsecond)

### Design Principles
- **Minimal Memory Allocation**: Predictable performance
- **Hash-based Lookup**: O(1) command resolution
- **Batch Processing**: Efficient multi-command execution
- **Telemetry Integration**: Cycle counting and validation

## Quick Start

### Basic Usage
```c
#include "cns.h"
#include "s7t.h"

// Initialize CNS engine
cns_cmd_entry_t commands[10];
uint32_t hash_table[256];
cns_engine_t engine;
cns_init(&engine, commands, hash_table, 10);

// Register commands
cns_register(&engine, "echo", echo_handler, CNS_FLAG_NONE, 0, 2, "Echo command");
cns_register(&engine, "help", help_handler, CNS_FLAG_NONE, 0, 1, "Help command");

// Parse and execute
cns_command_t cmd;
cns_parse("echo hello world", &cmd);
cns_execute(&engine, &cmd);
```

### CLI Integration
```c
#include "cns/cli.h"

// Initialize CLI
cns_cli_init("my_program");

// Register domains
CNSDomain domain = {
    .name = "file",
    .description = "File operations",
    .commands = file_commands,
    .command_count = 3
};
cns_cli_register_domain(&domain);

// Run CLI
cns_cli_run(argc, argv);
```

## Documentation Structure

### Core Documentation
- [**API Reference**](api/README.md) - Complete API documentation
- [**Architecture Guide**](architecture/README.md) - System design and principles
- [**Performance Guide**](performance/README.md) - Optimization and benchmarking
- [**Integration Guide**](integration/README.md) - System integration patterns
- [**Weaver System**](weaver/README.md) - OpenTelemetry code generation from TTL

### Developer Guides
- [**Getting Started**](getting-started/README.md) - Quick start tutorials
- [**Command Development**](commands/README.md) - Creating custom commands
- [**Testing Guide**](testing/README.md) - Unit testing and validation
- [**Debugging Guide**](debugging/README.md) - Troubleshooting and profiling

### Reference Materials
- [**Examples**](examples/README.md) - Code examples and patterns
- [**Best Practices**](best-practices/README.md) - Development guidelines
- [**FAQ**](faq/README.md) - Frequently asked questions
- [**Changelog**](changelog.md) - Version history and changes
- [**Weaver Documentation**](WEAVER.md) - Complete weaver system guide

## Key Features

### High Performance
- **Sub-microsecond execution** for most operations
- **Hash-based command lookup** for O(1) performance
- **Minimal memory allocation** for predictable timing
- **Cycle-accurate telemetry** for performance validation

### Developer Friendly
- **Simple registration API** for command handlers
- **Comprehensive help system** with automatic generation
- **Built-in debugging tools** for development
- **Extensive unit test suite** for validation

### Production Ready
- **Error handling** with detailed error codes
- **Permission system** for security
- **Batch processing** for efficiency
- **Performance monitoring** with metrics collection

## System Requirements

### Compiler Support
- **GCC 4.9+** with C99 support
- **Clang 3.5+** with C99 support
- **MSVC 2015+** with C99 support

### Dependencies
- **S7T Library** - 7-tick performance library
- **Standard C Library** - Basic system functions
- **Math Library** - For statistical calculations

### Platform Support
- **Linux** - Primary development platform
- **macOS** - Full compatibility
- **Windows** - Limited compatibility (requires S7T support)

## Getting Help

### Documentation
- [**API Reference**](api/README.md) - Complete function documentation
- [**Examples**](examples/README.md) - Working code examples
- [**Tutorials**](getting-started/README.md) - Step-by-step guides

### Support
- **Unit Tests** - Comprehensive test suite in `../tests/`
- **Performance Benchmarks** - Built-in benchmarking tools
- **Debug Tools** - Integrated debugging and profiling

### Community
- **Code Examples** - See `examples/` directory
- **Best Practices** - Development guidelines
- **FAQ** - Common questions and answers

## License

CNS is part of the 7-tick engine suite and follows the same licensing terms as the parent project.

---

*CNS provides ultra-low latency command processing with 7-tick compliance and comprehensive developer tooling.* 