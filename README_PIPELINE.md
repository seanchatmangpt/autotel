# AutoTel Semantic Execution Pipeline

## Overview

This is the **80/20 happy path implementation** of the AutoTel semantic execution pipeline, demonstrating the complete multi-compiler architecture: **processor > compiler > linker > executor**.

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

## Quick Start

### 1. Install Dependencies

```bash
# Install required packages
pip install rdflib lxml
```

### 2. Run the Test

```bash
# Test the complete pipeline
python test_pipeline.py
```

### 3. Use the CLI

```bash
# Run with example files
python autotel_cli.py \
  --owl examples/sample_owl.xml \
  --shacl examples/sample_shacl.xml \
  --dspy examples/sample_dspy.xml \
  --inputs examples/sample_inputs.json
```

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

### Programmatic Usage

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

### CLI Usage

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

## Output Format

The pipeline returns a structured result with:

```python
@dataclass
class ExecutionResult:
    success: bool                    # Execution success status
    outputs: Dict[str, Any]         # Generated outputs
    telemetry: Dict[str, Any]       # Execution telemetry
    validation_results: Dict[str, Any]  # Validation results
    execution_time: float           # Execution time in seconds
    metadata: Dict[str, Any]        # Execution metadata
```

## Key Features

### ✅ Implemented (80/20)

- **Multi-Compiler Architecture**: Clean separation between processors, compilers, linker, and executor
- **Semantic Type Classification**: Automatic classification of OWL classes (user_input, recommendation, decision, etc.)
- **Validation Integration**: SHACL constraints integrated into execution
- **Telemetry Generation**: Comprehensive execution telemetry
- **Fail-Fast Error Handling**: No try-catch blocks, errors crash naturally
- **Type-Safe Interfaces**: All components use structured dataclasses
- **CLI Interface**: Command-line interface for easy usage
- **File-Based Execution**: Support for XML files and JSON inputs
- **Test Coverage**: Complete test script demonstrating functionality

### 🔄 Future Enhancements

- **Real DSPy Integration**: Connect to actual DSPy library for execution
- **Advanced SHACL Processing**: Full SHACL constraint extraction and validation
- **OpenTelemetry Integration**: Real telemetry collection and export
- **Model Provider Integration**: Connect to actual LLM providers
- **Performance Optimization**: Parallel processing and caching
- **Advanced Error Handling**: Detailed error reporting and recovery
- **Configuration Management**: Hierarchical configuration system

## Design Principles

1. **Single Responsibility**: Each component has one clear purpose
2. **Fail Fast**: No try-catch blocks, let errors crash naturally
3. **Type Safety**: Use dataclasses for all data structures
4. **Clean Dependencies**: Clear separation between components
5. **Telemetry First**: Comprehensive observability throughout

## Testing

Run the complete test suite:

```bash
python test_pipeline.py
```

This will test:
- In-memory pipeline execution
- File-based pipeline execution
- All component integrations
- Error handling
- Output validation

## Contributing

This is a prototype implementation demonstrating the multi-compiler architecture. For production use, consider:

1. Adding comprehensive error handling
2. Implementing real DSPy integration
3. Adding OpenTelemetry support
4. Enhancing SHACL processing
5. Adding performance optimizations

## License

This implementation is part of the AutoTel project and follows the project's licensing terms. 