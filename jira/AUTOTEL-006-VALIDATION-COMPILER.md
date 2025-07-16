# AUTOTEL-006: Validation Compiler Implementation

## Summary
Implement the Validation compiler component that transforms SHACL constraints into structured validation rules for DSPy signature validation.

## Description
The Validation Compiler is the second compiler stage in the AutoTel execution pipeline (`processor > compiler > linker > executor`). It takes SHACL constraint graphs and compiles them into structured validation rules that will be used by the DSPy compiler to generate validation-aware signatures.

## Acceptance Criteria

### Functional Requirements
- [ ] Compile SHACL constraint graphs into structured validation rules
- [ ] Extract NodeShape definitions with target class mappings
- [ ] Extract PropertyShape constraints (minCount, maxCount, datatype, etc.)
- [ ] Generate validation rules for cardinality, data types, and value constraints
- [ ] Support SHACL Core constraint types and logical operators
- [ ] Preserve constraint severity levels (Violation, Warning, Info)
- [ ] Handle nested SHACL shapes and complex constraint hierarchies
- [ ] Generate human-readable validation messages

### Technical Requirements
- [ ] No try-catch blocks - let errors crash naturally
- [ ] Use structured dataclasses for all outputs
- [ ] Support SHACL Core constraint types
- [ ] Handle property paths and target class references
- [ ] Extract constraint severity levels
- [ ] Maintain constraint metadata for downstream use

### Data Structures
```python
@dataclass
class ValidationRules:
    target_classes: Dict[str, List[ValidationRule]]
    constraint_count: int
    severity_levels: Dict[str, int]
    metadata: Dict[str, Any]

@dataclass
class ValidationRule:
    rule_id: str
    target_class: str
    property_path: str
    constraint_type: str  # cardinality, datatype, value, logical
    constraint_value: Any
    severity: str  # Violation, Warning, Info
    message: str
    metadata: Dict[str, Any]

@dataclass
class ConstraintMetadata:
    shacl_triples: int
    node_shapes: int
    property_shapes: int
    constraint_types: List[str]
```

## Implementation Notes

### Constraint Type Extraction
The compiler should extract and classify constraints:
- **Cardinality**: minCount, maxCount, qualifiedMinCount, qualifiedMaxCount
- **Data Types**: datatype, nodeKind, class
- **Value Constraints**: minLength, maxLength, pattern, uniqueLang
- **Logical**: and, or, not, xone
- **Comparison**: equals, disjoint, lessThan, lessThanOrEquals

### Validation Rule Generation
- Convert SHACL constraints to structured validation rules
- Preserve constraint severity and types
- Generate human-readable validation messages
- Support nested constraint hierarchies

### Severity Level Handling
- **Violation**: Critical constraints that must be satisfied
- **Warning**: Important constraints that should be satisfied
- **Info**: Informational constraints for guidance

### Integration Points
- **Input**: Graph from SHACL processor
- **Output**: ValidationRules for DSPy compiler consumption
- **Dependencies**: SHACL processor
- **No Dependencies**: OWL, DSPy, BPMN, DMN (prototype phase only)

## Test Cases

### Basic Constraint Compilation
```python
# Test compilation from SHACL processor output
shacl_graph = shacl_processor.parse(xml_fragment)

validation_rules = validation_compiler.compile(shacl_graph=shacl_graph)
```

### Constraint Type Extraction
- Test cardinality constraint extraction
- Test data type constraint extraction
- Test value constraint extraction
- Test logical constraint extraction
- Test comparison constraint extraction

### Severity Level Handling
- Test Violation level constraints
- Test Warning level constraints
- Test Info level constraints
- Test default severity assignment

### Complex Constraint Hierarchies
- Test nested constraint structures
- Test property path resolution
- Test target class mapping
- Test constraint metadata preservation

## Definition of Done
- [ ] All acceptance criteria met
- [ ] Unit tests pass with >90% coverage
- [ ] Integration tests with DSPy compiler pass
- [ ] Documentation updated
- [ ] Code review completed
- [ ] No OWL/DSPy/BPMN/DMN dependencies in prototype phase

## Story Points
**3** - Medium complexity due to SHACL constraint parsing and rule generation

## Priority
**High** - Validation compiler component for the processor > compiler > linker > executor pipeline

## Labels
- `compiler`
- `validation`
- `shacl`
- `prototype`
- `constraints`

## Implementation Files
- `autotel/factory/validation_compiler.py` - Main validation compiler implementation
- `autotel/schemas/validation_types.py` - ValidationRules and related dataclasses
- `tests/test_factory_validation_compiler.py` - Unit tests for validation compilation
- `tests/test_validation_integration.py` - Integration tests with DSPy compiler

## NotImplementedError Implementations

The following `NotImplementedError` exceptions must be implemented in `autotel/factory/validation_compiler.py`:

### Core Compilation Methods
1. **`compile()`** - "SHACL validation compilation must be implemented with real compilation logic"
   - Transform SHACL graph into `ValidationRules` structure
   - Extract node shapes and property shapes
   - Generate validation rules with constraints
   - Return structured validation schema

### Shape Extraction Methods
2. **`_extract_node_shapes()`** - "Node shape extraction must be implemented with real extraction logic"
   - Find all SHACL node shapes in the graph
   - Extract target classes and constraints
   - Handle complex shape definitions
   - Return list of validation rules

3. **`_extract_property_shapes()`** - "Property shape extraction must be implemented with real extraction logic"
   - Find all SHACL property shapes in the graph
   - Extract property paths and constraints
   - Handle nested property shapes
   - Return list of validation rules

4. **`_extract_constraints()`** - "Constraint extraction must be implemented with real extraction logic"
   - Extract SHACL constraints from shape elements
   - Handle cardinality, datatype, value, and logical constraints
   - Convert to validation rule format
   - Return structured constraint data

### Rule Creation Methods
5. **`_create_validation_rule()`** - "Validation rule creation must be implemented with real creation logic"
   - Create `ValidationRule` objects from shape data
   - Set rule ID, target class, property path, and constraints
   - Handle severity levels and messages
   - Generate metadata for rule tracking

### Implementation Requirements
- Parse SHACL/RDF graph structure
- Extract node and property shapes
- Convert SHACL constraints to validation rules
- Handle complex SHACL constructs
- Support multiple constraint types
- Generate structured validation metadata
- No try-catch blocks - fail fast on errors
- Comprehensive error handling for malformed SHACL 