# AutoTel Semantic Execution Pipeline

## Overview

This is the **architecture foundation** for the AutoTel semantic execution pipeline, demonstrating the complete multi-compiler architecture: **processor > compiler > linker > executor**.

**⚠️ Current Status**: This is a **foundation implementation** with all core components defined as stubs. See [GAPS_AND_MISTAKES.md](./GAPS_AND_MISTAKES.md) for detailed implementation status.

## Architecture

The pipeline follows a clean separation of concerns:

```
XML Inputs → Processors → Compilers → Linker → Executor → Results + Telemetry
```

### Components

1. **Processors** (XML → Data Objects)
   - `OWLProcessor`: Parses OWL/RDF XML into structured ontology definitions
   - `SHACLProcessor`: Parses SHACL/RDF XML into constraint graphs
   - `DSPyProcessor`: Parses DSPy XML into signature definitions

2. **Compilers** (Data Objects → Execution-Ready Objects)
   - `OntologyCompiler`: Transforms OWL definitions into semantic schemas
   - `ValidationCompiler`: Transforms SHACL constraints into validation rules
   - `DSPyCompiler`: Integrates all inputs into execution-ready signatures

3. **Linker** (Execution-Ready Objects → Executable System)
   - `SemanticLinker`: Creates executable systems with semantic context

4. **Executor** (Executable System → Results + Telemetry)
   - `OntologyExecutor`: Runs systems with comprehensive telemetry

## Implementation Status

### ✅ **Completed**
- **Architecture Design**: Complete C4 diagrams and component interfaces
- **Data Structures**: All dataclasses and type definitions
- **Component Interfaces**: All method signatures and contracts
- **Documentation**: Comprehensive JIRA tickets and implementation guides
- **Test Framework**: Test structure and validation framework

### 🔄 **In Progress** (50+ NotImplementedError methods)
- **OWL Processor**: 15+ methods need implementation
- **SHACL Processor**: All parsing methods need implementation
- **DSPy Processor**: All processing methods need implementation
- **Ontology Compiler**: 7 methods need implementation
- **Validation Compiler**: 5 methods need implementation
- **DSPy Compiler**: 5 methods need implementation
- **Semantic Linker**: 5 methods need implementation
- **Ontology Executor**: 6 methods need implementation

### 📋 **Implementation Priority**
See [jira/PIPELINE-INDEX.md](./jira/PIPELINE-INDEX.md) for detailed implementation tickets.

1. **AUTOTEL-001**: OWL Processor (Foundation)
2. **AUTOTEL-002**: SHACL Processor (Validation)
3. **AUTOTEL-003**: Ontology Compiler (Schema Generation)
4. **AUTOTEL-006**: Validation Compiler (Constraint Rules)
5. **AUTOTEL-007**: DSPy Compiler (Integration)
6. **AUTOTEL-004**: Semantic Linker (System Integration)
7. **AUTOTEL-005**: Ontology Executor (Runtime)

## Quick Start

### 1. Install Dependencies

```bash
# Install required packages
pip install rdflib lxml
```

### 2. Review Implementation Status

```bash
# Check current gaps and implementation needs
cat GAPS_AND_MISTAKES.md

# Review JIRA tickets for implementation details
ls jira/
```

### 3. Run Tests (Currently Stubs)

```bash
# Test the complete pipeline (currently NotImplementedError stubs)
python test_pipeline.py
```

**Note**: All tests currently raise `NotImplementedError` - they need implementation before the pipeline can function.

## File Structure

```
autotel/
├── schemas/                    # Data structures
│   ├── ontology_types.py      # OWL/Ontology schemas
│   ├── validation_types.py    # SHACL/Validation schemas
│   ├── dspy_types.py         # DSPy schemas
│   └── linker_types.py       # Linker schemas
├── factory/                   # Pipeline components
│   ├── pipeline.py           # Main orchestrator
│   ├── processors/           # XML processors
│   │   ├── owl_processor.py
│   │   ├── shacl_processor.py
│   │   └── dspy_processor.py
│   ├── ontology_compiler.py  # Ontology compiler
│   ├── validation_compiler.py # Validation compiler
│   ├── dspy_compiler.py      # DSPy compiler
│   ├── linker.py             # Semantic linker
│   └── executor.py           # Ontology executor
├── autotel_cli.py            # CLI interface
├── test_pipeline.py          # Test script
└── examples/                 # Sample files
    ├── sample_owl.xml
    ├── sample_shacl.xml
    ├── sample_dspy.xml
    └── sample_inputs.json
```

## Usage Examples

### Programmatic Usage (When Implemented)

```python
from autotel.factory.pipeline import PipelineOrchestrator

# Initialize pipeline
pipeline = PipelineOrchestrator()

# Execute with XML strings
result = pipeline.execute_pipeline(
    owl_xml="<rdf:RDF>...</rdf:RDF>",
    shacl_xml="<rdf:RDF>...</rdf:RDF>",
    dspy_xml="<dspy:config>...</dspy:config>",
    inputs={"user_input": "Hello world"}
)

# Execute with files
result = pipeline.execute_from_files(
    owl_file="ontology.owl",
    shacl_file="constraints.shacl",
    dspy_file="signature.dspy",
    inputs={"user_input": "Hello world"}
)

# Access results
print(f"Success: {result.success}")
print(f"Outputs: {result.outputs}")
print(f"Telemetry: {result.telemetry}")
```

**Note**: This code will currently raise `NotImplementedError` - all methods need implementation.

### CLI Usage (When Implemented)

```bash
# Basic usage
python autotel_cli.py \
  --owl ontology.owl \
  --shacl constraints.shacl \
  --dspy signature.dspy \
  --inputs data.json

# With output file
python autotel_cli.py \
  --owl ontology.owl \
  --shacl constraints.shacl \
  --dspy signature.dspy \
  --inputs data.json \
  --output results.json
```

**Note**: CLI currently calls non-functional pipeline - needs implementation.

## Input Formats

### OWL/RDF XML

```xml
<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:owl="http://www.w3.org/2002/07/owl#"
         xmlns:rdfs="http://www.w3.org/2000/01/rdf-schema#">
    <owl:Ontology rdf:about="http://example.org/ontology"/>
    <owl:Class rdf:about="#UserInput">
        <rdfs:label>User Input</rdfs:label>
        <rdfs:comment>Input data from user</rdfs:comment>
    </owl:Class>
</rdf:RDF>
```

### SHACL/RDF XML

```xml
<?xml version="1.0" encoding="UTF-8"?>
<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
         xmlns:shacl="http://www.w3.org/ns/shacl#">
    <shacl:NodeShape rdf:about="#UserInputShape">
        <shacl:targetClass rdf:resource="#UserInput"/>
        <shacl:property>
            <shacl:PropertyShape>
                <shacl:path rdf:resource="#hasText"/>
                <shacl:minCount>1</shacl:minCount>
                <shacl:maxCount>1</shacl:maxCount>
            </shacl:PropertyShape>
        </shacl:property>
    </shacl:NodeShape>
</rdf:RDF>
```

### DSPy XML

```xml
<?xml version="1.0" encoding="UTF-8"?>
<dspy:config xmlns:dspy="http://autotel.ai/dspy">
    <dspy:signature name="test_signature" description="Test signature">
        <dspy:input name="user_input" type="string" description="User input"/>
        <dspy:output name="recommendation" type="string" description="Recommendation"/>
    </dspy:signature>
    <dspy:module type="predict">
        <dspy:config key="temperature" value="0.7"/>
    </dspy:module>
    <dspy:model provider="openai" name="gpt-4">
        <dspy:parameter key="temperature" value="0.7"/>
    </dspy:model>
</dspy:config>
```

### Input JSON

```json
{
  "UserInput": "I want to learn machine learning",
  "user_input": "I want to learn machine learning"
}
```

## Implementation Guide

### Getting Started

1. **Review Gaps**: Read [GAPS_AND_MISTAKES.md](./GAPS_AND_MISTAKES.md) for current status
2. **Choose Ticket**: Pick a JIRA ticket from [jira/PIPELINE-INDEX.md](./jira/PIPELINE-INDEX.md)
3. **Implement**: Follow the detailed implementation guide in each ticket
4. **Test**: Update tests to validate your implementation
5. **Document**: Update documentation as needed

### Development Workflow

1. **Start with Processors**: Implement OWL and SHACL processors first
2. **Move to Compilers**: Implement ontology and validation compilers
3. **Complete Integration**: Implement DSPy compiler and semantic linker
4. **Add Execution**: Implement ontology executor
5. **Test End-to-End**: Implement comprehensive tests

### Key Principles

- **Fail Fast**: No try-catch blocks - let errors crash naturally
- **Type Safety**: Use strong typing with dataclasses throughout
- **Telemetry First**: Generate comprehensive OpenTelemetry traces
- **Semantic Context**: Preserve semantic context throughout pipeline
- **Validation**: Apply SHACL constraints at runtime

## Related Documents

- [GAPS_AND_MISTAKES.md](./GAPS_AND_MISTAKES.md) - Current implementation gaps
- [jira/PIPELINE-INDEX.md](./jira/PIPELINE-INDEX.md) - Implementation tickets
- [c4_validation_diagrams.md](./c4_validation_diagrams.md) - Architecture validation
- [pipeline_sequence_diagrams.md](./pipeline_sequence_diagrams.md) - Sequence flows

## Contributing

1. **Pick a Ticket**: Choose from the JIRA tickets in priority order
2. **Follow Implementation Guide**: Each ticket has detailed implementation steps
3. **Implement Tests**: Update tests to validate your implementation
4. **Update Documentation**: Keep documentation current with implementation
5. **Submit PR**: Create pull request with implementation and tests

## Support

- **Architecture Questions**: Review C4 diagrams and sequence diagrams
- **Implementation Questions**: Check JIRA tickets for detailed guides
- **Gap Analysis**: See GAPS_AND_MISTAKES.md for current status
- **Testing**: All tests need implementation - see test files for structure 