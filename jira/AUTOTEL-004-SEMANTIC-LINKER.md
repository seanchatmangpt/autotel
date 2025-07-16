# AUTOTEL-004: Semantic Linker Implementation

## Summary
Implement the semantic linker component that creates executable systems from compiled DSPy signatures with ontology context and validation rules.

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