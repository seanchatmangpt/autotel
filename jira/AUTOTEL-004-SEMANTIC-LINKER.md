# AUTOTEL-004: Semantic Linker Implementation

## 🎯 **TELEMETRY IMPLEMENTATION REQUIREMENTS**

### **Required Spans and Events**
The following telemetry must be implemented to validate real functionality:

#### **Main Linking Span**
```yaml
span_name: "semantic.linker.link"
required_attributes:
  - component: "semantic_linker"
  - operation: "link"
  - input_format: "DSPySignature"
  - output_format: "ExecutableSystem"
  - success: boolean
  - signature_validated: boolean
  - semantic_context_generated: boolean
  - validation_rules_prepared: boolean
  - metadata_generated: boolean
  - executable_system_created: boolean
  - linking_duration_ms: integer
  - input_signature_count: integer
  - output_system_count: integer

required_events:
  - "linking_started"
  - "signature_validated"
  - "semantic_context_generated"
  - "validation_rules_prepared"
  - "metadata_generated"
  - "executable_system_created"
```

#### **Linking Method Spans**
```yaml
span_name: "semantic.linker.validate_signature"
required_attributes:
  - signature_name: string
  - inputs_validated: integer
  - outputs_validated: integer
  - validation_errors: list
  - signature_valid: boolean

span_name: "semantic.linker.generate_semantic_context"
required_attributes:
  - context_generated: boolean
  - ontology_classes_used: list
  - semantic_types_mapped: dict
  - context_size_bytes: integer

span_name: "semantic.linker.prepare_validation_rules"
required_attributes:
  - rules_prepared: integer
  - rule_types: list
  - validation_targets: list
  - rules_active: boolean

span_name: "semantic.linker.generate_metadata"
required_attributes:
  - metadata_generated: boolean
  - execution_context: dict
  - telemetry_config: dict
  - metadata_size_bytes: integer
```

### **Dynamic Data Validation**
The following dynamic data must be generated from real signature linking:

#### **Expected Dynamic Data from Sample DSPy Signature Input**
```yaml
# Input: DSPySignature
input:
  name: "recommendation_signature"
  description: "Generate recommendations from user input"
  inputs:
    user_input:
      name: "user_input"
      type: "string"
      description: "User input text"
      semantic_type: "user_input"
  outputs:
    recommendation:
      name: "recommendation"
      type: "string"
      description: "AI-generated recommendation"
      semantic_type: "recommendation"
    confidence:
      name: "confidence"
      type: "float"
      description: "Confidence score"
      semantic_type: "recommendation"
  ontology_context:
    user_input_classes: ["UserInput"]
    recommendation_classes: ["Recommendation"]
  validation_rules:
    - rule_type: "min_length"
      target: "user_input"
      value: 1
    - rule_type: "range"
      target: "confidence"
      min: 0.0
      max: 1.0
```

#### **Expected Telemetry Data**
```yaml
# Span: semantic.linker.link
attributes:
  success: true
  signature_validated: true
  semantic_context_generated: true
  validation_rules_prepared: true
  metadata_generated: true
  executable_system_created: true
  linking_duration_ms: 25
  input_signature_count: 1
  output_system_count: 1

# Span: semantic.linker.validate_signature
attributes:
  signature_name: "recommendation_signature"
  inputs_validated: 1
  outputs_validated: 2
  validation_errors: []
  signature_valid: true

# Span: semantic.linker.generate_semantic_context
attributes:
  context_generated: true
  ontology_classes_used: ["UserInput", "Recommendation"]
  semantic_types_mapped:
    user_input: "user_input"
    recommendation: "recommendation"
    confidence: "recommendation"
  context_size_bytes: 512

# Span: semantic.linker.prepare_validation_rules
attributes:
  rules_prepared: 2
  rule_types: ["min_length", "range"]
  validation_targets: ["user_input", "confidence"]
  rules_active: true

# Span: semantic.linker.generate_metadata
attributes:
  metadata_generated: true
  execution_context:
    signature_name: "recommendation_signature"
    model_provider: "openai"
    temperature: 0.7
  telemetry_config:
    enabled: true
    sampling_rate: 1.0
  metadata_size_bytes: 256

# Expected ExecutableSystem output
output:
  signature:
    name: "recommendation_signature"
    description: "Generate recommendations from user input"
    inputs:
      user_input:
        name: "user_input"
        type: "string"
        description: "User input text"
        semantic_type: "user_input"
        validation_rules: ["min_length:1"]
    outputs:
      recommendation:
        name: "recommendation"
        type: "string"
        description: "AI-generated recommendation"
        semantic_type: "recommendation"
      confidence:
        name: "confidence"
        type: "float"
        description: "Confidence score"
        semantic_type: "recommendation"
        validation_rules: ["range:0.0:1.0"]
  semantic_context:
    ontology_classes: ["UserInput", "Recommendation"]
    semantic_types:
      user_input: "user_input"
      recommendation: "recommendation"
  validation_rules:
    - rule_type: "min_length"
      target: "user_input"
      value: 1
    - rule_type: "range"
      target: "confidence"
      min: 0.0
      max: 1.0
  metadata:
    execution_context:
      signature_name: "recommendation_signature"
      model_provider: "openai"
      temperature: 0.7
    telemetry_config:
      enabled: true
      sampling_rate: 1.0
```

### **Validation Criteria**
- **NO HARDCODED VALUES**: All telemetry data must be generated from actual signature linking
- **REAL LINKING**: DSPy signatures must be transformed into executable systems
- **DYNAMIC VALIDATION**: All validation results must come from actual signature analysis
- **CONTEXT GENERATION**: Semantic context must be derived from signature data
- **RULE PREPARATION**: Validation rules must be extracted and prepared from signature
- **ERROR HANDLING**: Failed linking must generate error spans with context
- **PERFORMANCE TRACKING**: Linking duration must be measured and reported
- **DATA INTEGRITY**: Input/output counts must match and be accurate

---

## Summary
Implement the Semantic Linker component of the AutoTel semantic execution pipeline to transform compiled DSPy signatures into executable systems with semantic context and validation rules.

## Description
The Semantic Linker is the third stage in the AutoTel execution pipeline (`processor > compiler > linker > executor`). It takes compiled DSPy signatures and creates fully connected executable systems ready for runtime execution with semantic context and validation.

## Acceptance Criteria

### Functional Requirements
- [ ] Link compiled DSPy signatures into executable system configurations
- [ ] Integrate ontology context for semantic execution
- [ ] Apply SHACL validation rules to executable systems
- [ ] Validate system integrity before execution
- [ ] Generate semantic context for runtime interpretation
- [ ] Preserve model configurations and module settings
- [ ] Support system validation and health checks

### Technical Requirements
- [ ] No try-catch blocks - let errors crash naturally
- [ ] Use structured dictionaries for executable system output
- [ ] Validate system components before linking
- [ ] Generate semantic context from ontology definitions
- [ ] Preserve validation rules for runtime enforcement
- [ ] Support system introspection and metadata

### Data Structures
```python
@dataclass
class ExecutableSystem:
    signature: DSPySignature
    ontology: Optional[OWLOntologyDefinition]
    semantic_context: Dict[str, Any]
    validation_rules: List[Dict[str, Any]]
    examples: List[Dict[str, Any]]
    model_config: Optional[DSPyModelConfiguration]
    module_config: Optional[DSPyModuleDefinition]
    shacl_validation: bool
    shacl_triples: int
```

## Implementation Notes

### System Linking Process
The linker should:
1. **Validate Inputs**: Ensure all components are valid and complete
2. **Generate Context**: Create semantic context from ontology
3. **Apply Validation**: Integrate SHACL constraints
4. **Preserve Configuration**: Maintain model and module settings
5. **Create System**: Generate executable system configuration

### Semantic Context Generation
- Extract ontology URI and namespace information
- List available classes and properties
- Preserve semantic type classifications
- Include property relationships and hierarchies

### Validation Integration
- Preserve SHACL constraint rules
- Include validation metadata
- Support runtime constraint checking
- Generate validation status information

### Integration Points
- **Input**: DSPySignature from DSPy compiler
- **Output**: Dict[str, Any] (executable system) for executor consumption
- **Dependencies**: DSPy compiler
- **No Dependencies**: OWL, SHACL, BPMN, DMN (prototype phase only)

## Test Cases

### Basic System Linking
```python
# Test linking from DSPy compiler output
signature = dspy_compiler.compile(
    ontology_schema=ontology_schema,
    validation_rules=validation_rules,
    dspy_signatures=dspy_signatures,
    dspy_modules=dspy_modules,
    model_config=model_config
)

linker = SemanticLinker()
executable_system = linker.link(signature)
```

### System Validation
- Test signature validation (name, description, inputs, outputs)
- Test semantic context validation
- Test model configuration validation
- Test module configuration validation

### Semantic Context Generation
- Test semantic context preservation from DSPy signature
- Test ontology metadata handling
- Test validation rule integration
- Test example data preservation

### Validation Rule Integration
- Test validation rule preservation from DSPy signature
- Test validation metadata generation
- Test constraint count tracking
- Test validation status reporting

## Definition of Done
- [ ] All acceptance criteria met
- [ ] Unit tests pass with >90% coverage
- [ ] Integration tests with compiler and executor pass
- [ ] Documentation updated
- [ ] Code review completed
- [ ] No BPMN/DMN dependencies in prototype phase

## Story Points
**5** - Medium complexity due to system integration and validation

## Priority
**High** - Integration component for the processor > compiler > linker > executor pipeline

## Labels
- `linker`
- `semantic`
- `integration`
- `prototype`
- `validation`

## Implementation Files
- `autotel/factory/linker.py` - Main semantic linker implementation
- `autotel/schemas/linker_types.py` - ExecutableSystem and related dataclasses
- `autotel/factory/pipeline.py` - Pipeline orchestration
- `tests/test_factory_linker.py` - Unit tests for semantic linking
- `tests/test_semantic_linker_integration.py` - Integration tests with compiler and executor 

## NotImplementedError Implementations

The following `NotImplementedError` exceptions must be implemented in `autotel/factory/linker.py`:

### Core Linking Methods
1. **`link()`** - "Semantic linking must be implemented with real linking logic"
   - Create executable system from DSPy signature
   - Generate semantic context and validation rules
   - Prepare examples and metadata
   - Return `ExecutableSystem` object

### Validation and Context Methods
2. **`_validate_signature()`** - "Signature validation must be implemented with real validation logic"
   - Validate DSPy signature completeness
   - Check required fields and structure
   - Verify input/output definitions
   - Ensure signature is executable

3. **`_generate_semantic_context()`** - "Semantic context generation must be implemented with real generation logic"
   - Extract semantic context from signature
   - Generate execution context metadata
   - Create semantic relationships
   - Return structured context data

4. **`_prepare_validation_rules()`** - "Validation rule preparation must be implemented with real preparation logic"
   - Prepare validation rules for execution
   - Convert rules to executable format
   - Generate validation metadata
   - Return prepared validation rules

### Metadata Generation Methods
5. **`_generate_metadata()`** - "Metadata generation must be implemented with real generation logic"
   - Generate comprehensive system metadata
   - Include ontology, model, and module information
   - Create execution tracking metadata
   - Return structured metadata object

### Implementation Requirements
- Create executable systems from DSPy signatures
- Generate semantic context for execution
- Prepare validation rules for runtime
- Create comprehensive metadata
- Support multiple signature types
- Handle complex semantic relationships
- No try-catch blocks - fail fast on errors
- Comprehensive error handling for malformed signatures 