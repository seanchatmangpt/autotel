# AUTOTEL-005: Ontology-Driven Executor Implementation

## Summary
Implement the ontology-driven executor component that runs DSPy signatures with semantic context, validation, and telemetry for automated AI execution.

## Description
The Ontology-Driven Executor is the final stage in the AutoTel execution pipeline (`processor > compiler > linker > executor`). It takes executable systems from the semantic linker and runs DSPy signatures with full semantic context, validation rules, and comprehensive telemetry.

## Acceptance Criteria

### Functional Requirements
- [ ] Execute DSPy signatures with semantic context from ontologies
- [ ] Apply SHACL validation rules during execution
- [ ] Generate comprehensive telemetry (spans, events, traces)
- [ ] Support DSPy Predict and ChainOfThought modules
- [ ] Handle model configuration and provider integration
- [ ] Validate inputs against semantic schemas
- [ ] Generate outputs with semantic type validation
- [ ] Support example-based execution with OWL individuals

### Technical Requirements
- [ ] No try-catch blocks - let errors crash naturally
- [ ] Use OpenTelemetry for comprehensive telemetry
- [ ] Support multiple DSPy model providers (OpenAI, Anthropic, Ollama)
- [ ] Apply validation rules at runtime
- [ ] Generate semantic-aware telemetry spans
- [ ] Support execution tracing and debugging
- [ ] Handle model configuration and caching

### Execution Features
- **Semantic Execution**: Use ontology context for intelligent execution
- **Validation**: Apply SHACL constraints during runtime
- **Telemetry**: Generate comprehensive execution traces
- **Examples**: Use OWL individuals as execution examples
- **Caching**: Support model response caching
- **Error Handling**: Semantic error reporting with context

## Implementation Notes

### Execution Flow
The executor should:
1. **Initialize**: Set up telemetry, validation, and model configuration
2. **Validate Inputs**: Apply SHACL constraints to input data
3. **Execute**: Run DSPy signatures with semantic context
4. **Validate Outputs**: Apply constraints to generated outputs
5. **Telemetry**: Generate comprehensive execution traces
6. **Return**: Structured results with validation status

### Telemetry Integration
- **Spans**: Execution spans with semantic context
- **Events**: Validation events and constraint checks
- **Traces**: Complete execution traces with ontology context
- **Metrics**: Performance and validation metrics

### Model Integration
- Support multiple DSPy model providers
- Handle model configuration and parameters
- Support response caching and optimization
- Generate model-specific telemetry

### Integration Points
- **Input**: Dict[str, Any] (executable system) from semantic linker
- **Output**: Execution results with telemetry and validation status
- **Dependencies**: DSPy, OpenTelemetry, model providers
- **No Dependencies**: BPMN, DMN (prototype phase only)

## Test Cases

### Basic Execution
```python
# Test execution from linker output
executable_system = linker.link(signature, ontology, shacl_graph)

executor = OntologyExecutor()
result = executor.execute(executable_system, input_data)
```

### Semantic Execution
- Test execution with ontology context
- Test semantic type validation
- Test property relationship handling
- Test class hierarchy resolution

### Validation Execution
- Test SHACL constraint application
- Test input validation
- Test output validation
- Test constraint violation handling

### Telemetry Generation
- Test execution span generation
- Test validation event creation
- Test trace context propagation
- Test metric collection

### Model Integration
- Test multiple model providers
- Test model configuration
- Test response caching
- Test error handling

## Definition of Done
- [ ] All acceptance criteria met
- [ ] Unit tests pass with >90% coverage
- [ ] Integration tests with linker pass
- [ ] Telemetry integration verified
- [ ] Documentation updated
- [ ] Code review completed
- [ ] No BPMN/DMN dependencies in prototype phase

## Story Points
**13** - High complexity due to execution, telemetry, and validation integration

## Priority
**High** - Runtime component for the processor > compiler > linker > executor pipeline

## Labels
- `executor`
- `ontology`
- `telemetry`
- `prototype`
- `runtime`

## Implementation Files
- `autotel/factory/executor.py` - Main ontology executor implementation
- `autotel/schemas/executor_types.py` - ExecutionResult and related dataclasses
- `autotel/core/telemetry.py` - Telemetry integration
- `tests/test_factory_executor.py` - Unit tests for ontology execution
- `tests/test_ontology_executor_integration.py` - Integration tests with telemetry 

## NotImplementedError Implementations

The following `NotImplementedError` exceptions must be implemented in `autotel/factory/executor.py`:

### Core Execution Methods
1. **`execute()`** - "OntologyExecutor.execute() must be implemented with real execution logic"
   - Execute DSPy signature with semantic context
   - Apply validation rules and generate results
   - Create telemetry and metadata
   - Return `ExecutionResult` object

### Validation Methods
2. **`_validate_inputs()`** - "Input validation must be implemented with real validation logic"
   - Validate inputs against signature definition
   - Check required fields and data types
   - Apply semantic type validation
   - Ensure input completeness

3. **`_apply_validation_rules()`** - "Validation rule application must be implemented with real validation logic"
   - Apply SHACL validation rules to inputs
   - Check constraint violations
   - Generate validation results
   - Return structured validation data

### Execution and Telemetry Methods
4. **`_execute_signature()`** - "DSPy signature execution must be implemented with real execution logic"
   - Execute DSPy signature with real model calls
   - Handle model provider integration
   - Process inputs and generate outputs
   - Return structured execution results

5. **`_generate_telemetry()`** - "Telemetry generation must be implemented with real telemetry logic"
   - Generate OpenTelemetry spans and events
   - Track execution performance metrics
   - Create semantic-aware telemetry
   - Return comprehensive telemetry data

6. **`_generate_execution_metadata()`** - "Metadata generation must be implemented with real metadata logic"
   - Generate execution metadata
   - Include timing, context, and performance data
   - Create execution tracking information
   - Return structured metadata object

### Implementation Requirements
- Execute DSPy signatures with real model calls
- Apply comprehensive validation rules
- Generate OpenTelemetry telemetry
- Handle multiple model providers
- Support semantic context execution
- Create detailed execution metadata
- No try-catch blocks - fail fast on errors
- Comprehensive error handling for execution failures 