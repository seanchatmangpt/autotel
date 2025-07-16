# AUTOTEL-007: DSPy Compiler Implementation

## Summary
Implement the DSPy compiler component that integrates ontology schemas, validation rules, and DSPy XML definitions into execution-ready DSPy signatures.

## Description
The DSPy Compiler is the final compiler stage in the AutoTel execution pipeline (`processor > compiler > linker > executor`). It takes compiled outputs from the Ontology and Validation compilers, along with DSPy XML definitions, and generates comprehensive DSPy signatures ready for execution.

## Acceptance Criteria

### Functional Requirements
- [ ] Integrate OntologySchema with DSPy signature definitions
- [ ] Apply ValidationRules to DSPy signature validation
- [ ] Map OWL classes to DSPy input/output fields based on semantic types
- [ ] Generate execution-ready `DSPySignature` objects
- [ ] Preserve DSPy module configurations and model settings
- [ ] Extract OWL individuals as DSPy signature examples
- [ ] Support multiple input sources (XML fragments, separate files)
- [ ] Generate semantic context for runtime interpretation

### Technical Requirements
- [ ] No try-catch blocks - let errors crash naturally
- [ ] Use structured dataclasses for all outputs
- [ ] Support semantic type-based schema generation
- [ ] Handle property inheritance and class hierarchies
- [ ] Generate validation rules from SHACL constraints
- [ ] Maintain ontology context for semantic linking

### Data Structures
```python
@dataclass
class DSPySignature:
    name: str
    description: str
    inputs: Dict[str, Dict[str, Any]]  # OWL classes mapped to input schemas
    outputs: Dict[str, Dict[str, Any]]  # OWL classes mapped to output schemas
    validation_rules: List[Dict[str, Any]]  # SHACL constraints
    examples: List[Dict[str, Any]]  # OWL individuals
    model_config: Optional[DSPyModelConfiguration]
    module_config: Optional[DSPyModuleDefinition]
    semantic_context: Dict[str, Any]
    ontology_metadata: Dict[str, Any]

@dataclass
class DSPyModelConfiguration:
    provider: str
    model_name: str
    parameters: Dict[str, Any]
    telemetry_config: Dict[str, Any]

@dataclass
class DSPyModuleDefinition:
    module_type: str
    configuration: Dict[str, Any]
    dependencies: List[str]
```

## Implementation Notes

### Semantic Type Mapping
The compiler should map OWL classes to DSPy fields based on semantic types:
- **user_input/general** → Input fields
- **recommendation/decision/analysis/reasoning** → Output fields

### Schema Integration
- Extract class properties as field schemas
- Map OWL data types to Python types
- Include semantic context and descriptions
- Preserve property relationships and constraints

### Validation Rule Integration
- Convert ValidationRules to DSPy signature validation
- Preserve constraint severity and types
- Generate human-readable validation messages
- Support nested constraint hierarchies

### DSPy Configuration Integration
- Preserve model provider configurations
- Maintain module settings and dependencies
- Include telemetry configuration
- Support multiple model providers

### Integration Points
- **Input**: OntologySchema, ValidationRules, DSPySignatureDefinition, DSPyModuleDefinition, DSPyModelConfiguration
- **Output**: DSPySignature for linker consumption
- **Dependencies**: Ontology compiler, Validation compiler, DSPy processor
- **No Dependencies**: BPMN, DMN (prototype phase only)

## Test Cases

### Basic Integration Compilation
```python
# Test compilation from all compiler outputs
ontology_schema = ontology_compiler.compile(ontology=ontology)
validation_rules = validation_compiler.compile(shacl_graph=shacl_graph)
dspy_signatures = dspy_processor.parse(xml_fragment)
dspy_modules = dspy_processor.parse_modules(xml_fragment)
model_config = dspy_processor.parse_model_configuration(xml_fragment)

signature = dspy_compiler.compile(
    ontology_schema=ontology_schema,
    validation_rules=validation_rules,
    dspy_signatures=dspy_signatures,
    dspy_modules=dspy_modules,
    model_config=model_config
)
```

### Semantic Type Mapping
- Test user_input classes mapped to inputs
- Test recommendation classes mapped to outputs
- Test general classes mapped appropriately
- Test edge cases and default behavior

### Validation Rule Integration
- Test SHACL constraint conversion
- Test cardinality rule generation
- Test data type validation
- Test complex constraint hierarchies

### Schema Generation
- Test property extraction and mapping
- Test data type conversion
- Test semantic context preservation
- Test example extraction from individuals

### Configuration Integration
- Test model configuration preservation
- Test module configuration integration
- Test telemetry configuration handling
- Test provider-specific settings

## Definition of Done
- [ ] All acceptance criteria met
- [ ] Unit tests pass with >90% coverage
- [ ] Integration tests with linker pass
- [ ] Documentation updated
- [ ] Code review completed
- [ ] No BPMN/DMN dependencies in prototype phase

## Story Points
**8** - High complexity due to multi-source integration and semantic mapping

## Priority
**High** - Final compiler component for the processor > compiler > linker > executor pipeline

## Labels
- `compiler`
- `dspy`
- `integration`
- `prototype`
- `signatures`

## Implementation Files
- `autotel/factory/dspy_compiler.py` - Main DSPy compiler implementation
- `autotel/schemas/dspy_types.py` - DSPySignature and related dataclasses
- `tests/test_factory_dspy_compiler.py` - Unit tests for DSPy compilation
- `tests/test_dspy_integration.py` - Integration tests with semantic linker

## NotImplementedError Implementations

The following `NotImplementedError` exceptions must be implemented in `autotel/factory/dspy_compiler.py`:

### Core Compilation Methods
1. **`compile()`** - "DSPy compilation must be implemented with real compilation logic"
   - Integrate ontology schema, validation rules, and DSPy components
   - Merge multiple DSPy signatures into single signature
   - Apply ontology context and validation rules
   - Return executable `DSPySignature`

### Signature Processing Methods
2. **`_merge_signatures()`** - "Signature merging must be implemented with real merging logic"
   - Combine multiple DSPy signature definitions
   - Resolve conflicts between signatures
   - Merge inputs, outputs, and examples
   - Return unified signature definition

3. **`_integrate_ontology_context()`** - "Ontology integration must be implemented with real integration logic"
   - Map ontology classes to DSPy inputs/outputs
   - Apply semantic type classification
   - Generate semantic context for signatures
   - Enhance signature with ontological information

4. **`_integrate_validation_rules()`** - "Validation integration must be implemented with real integration logic"
   - Convert validation rules to DSPy format
   - Apply constraints to signature inputs/outputs
   - Generate validation metadata
   - Integrate with signature validation

### Signature Creation Methods
5. **`_create_dspy_signature()`** - "DSPy signature creation must be implemented with real creation logic"
   - Create `DSPySignature` from components
   - Set signature name, description, and metadata
   - Configure model and module settings
   - Return executable signature object

### Implementation Requirements
- Integrate ontology, validation, and DSPy components
- Handle multiple signature definitions
- Apply semantic context and validation rules
- Generate executable DSPy signatures
- Support model and module configuration
- Create comprehensive signature metadata
- No try-catch blocks - fail fast on errors
- Comprehensive error handling for malformed data 