# AUTOTEL-005: Ontology Executor Implementation

## 🎯 **TELEMETRY IMPLEMENTATION REQUIREMENTS**

### **Required Spans and Events**
The following telemetry must be implemented to validate real functionality:

#### **Main Execution Span**
```yaml
span_name: "ontology.executor.execute"
required_attributes:
  - component: "ontology_executor"
  - operation: "execute"
  - input_format: "ExecutableSystem"
  - output_format: "ExecutionResult"
  - success: boolean
  - inputs_validated: boolean
  - validation_rules_applied: boolean
  - dspy_signature_executed: boolean
  - model_provider_called: boolean
  - outputs_generated: boolean
  - telemetry_generated: boolean
  - execution_duration_ms: integer
  - input_count: integer
  - output_count: integer
  - validation_violations: integer

required_events:
  - "execution_started"
  - "inputs_validated"
  - "validation_rules_applied"
  - "dspy_signature_executed"
  - "model_provider_called"
  - "outputs_generated"
  - "telemetry_generated"
  - "execution_completed"
```

#### **Execution Method Spans**
```yaml
span_name: "ontology.executor.validate_inputs"
required_attributes:
  - inputs_validated: integer
  - validation_errors: list
  - validation_passed: boolean
  - input_types_checked: list

span_name: "ontology.executor.apply_validation_rules"
required_attributes:
  - rules_applied: integer
  - rules_passed: integer
  - rules_failed: integer
  - violations_detected: list
  - validation_success: boolean

span_name: "ontology.executor.execute_signature"
required_attributes:
  - signature_name: string
  - model_provider: string
  - model_name: string
  - execution_success: boolean
  - response_time_ms: integer
  - token_count: integer

span_name: "ontology.executor.generate_telemetry"
required_attributes:
  - telemetry_generated: boolean
  - spans_created: integer
  - events_emitted: integer
  - metrics_recorded: integer
  - telemetry_size_bytes: integer
```

### **Dynamic Data Validation**
The following dynamic data must be generated from real signature execution:

#### **Expected Dynamic Data from Sample Execution**
```yaml
# Input: ExecutableSystem
input:
  signature:
    name: "recommendation_signature"
    inputs:
      user_input:
        name: "user_input"
        type: "string"
        validation_rules: ["min_length:1"]
    outputs:
      recommendation:
        name: "recommendation"
        type: "string"
      confidence:
        name: "confidence"
        type: "float"
        validation_rules: ["range:0.0:1.0"]
  validation_rules:
    - rule_type: "min_length"
      target: "user_input"
      value: 1
    - rule_type: "range"
      target: "confidence"
      min: 0.0
      max: 1.0

# User Input
user_inputs:
  user_input: "I need help with Python programming"
```

#### **Expected Telemetry Data**
```yaml
# Span: ontology.executor.execute
attributes:
  success: true
  inputs_validated: true
  validation_rules_applied: true
  dspy_signature_executed: true
  model_provider_called: true
  outputs_generated: true
  telemetry_generated: true
  execution_duration_ms: 2500
  input_count: 1
  output_count: 2
  validation_violations: 0

# Span: ontology.executor.validate_inputs
attributes:
  inputs_validated: 1
  validation_errors: []
  validation_passed: true
  input_types_checked: ["string"]

# Span: ontology.executor.apply_validation_rules
attributes:
  rules_applied: 2
  rules_passed: 2
  rules_failed: 0
  violations_detected: []
  validation_success: true

# Span: ontology.executor.execute_signature
attributes:
  signature_name: "recommendation_signature"
  model_provider: "openai"
  model_name: "gpt-4"
  execution_success: true
  response_time_ms: 2000
  token_count: 150

# Span: ontology.executor.generate_telemetry
attributes:
  telemetry_generated: true
  spans_created: 5
  events_emitted: 8
  metrics_recorded: 3
  telemetry_size_bytes: 1024

# Expected ExecutionResult output
output:
  success: true
  outputs:
    recommendation: "Consider learning Python basics first, then move to advanced topics like data structures and algorithms. Start with online tutorials and practice coding regularly."
    confidence: 0.85
  validation_results:
    input_validation: "PASSED"
    rule_validation: "PASSED"
    violations: []
  execution_time: 2.5
  telemetry:
    spans_generated: 5
    events_emitted: 8
    metrics_recorded: 3
  metadata:
    signature_name: "recommendation_signature"
    model_provider: "openai"
    model_name: "gpt-4"
    token_count: 150
    response_time_ms: 2000
```

### **Validation Criteria**
- **NO HARDCODED VALUES**: All telemetry data must be generated from actual execution
- **REAL EXECUTION**: DSPy signatures must be executed with real model providers
- **DYNAMIC VALIDATION**: All validation results must come from actual input validation
- **MODEL INTEGRATION**: Real model provider calls must be made and tracked
- **PERFORMANCE TRACKING**: Execution time, response time, and token counts must be measured
- **ERROR HANDLING**: Failed execution must generate error spans with context
- **TELEMETRY GENERATION**: Real telemetry must be generated during execution
- **DATA INTEGRITY**: Input/output counts and validation results must be accurate

---

## Summary
Implement the Ontology Executor component of the AutoTel semantic execution pipeline to execute DSPy signatures with semantic context, validation rules, and comprehensive telemetry.

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